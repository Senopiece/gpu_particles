#define SFML_STATIC
#define GLEW_STATIC

#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector4.hpp>
#include <SFML/System/Clock.hpp>
#include <windows.h>
#include <fstream>
#include <vector>

#include "labels.h"
#include "coordinates_mapping.h"
#include "open_gl_related.h"

using namespace std;
using namespace sf;

typedef unsigned int uint;
typedef Vector2f vec2;
typedef Vector4f vec4;
typedef vec4 particle;

///  C O M M O N  V A R I A B L E S  ///

RenderWindow* window;

Font default_font;

FPS_Label fps_label(default_font);
NotificationLabel notification_label(default_font);

Clock notify_fade_timer;

uint frag_shader_id = 0;
uint cur_prog_id = 0;

int   spawn_amount = 100;
float spawn_radius = 40;

vec2* prepared_spawn = new vec2[spawn_amount];

uint takt = 0;
uint particles_count = 0;  // particles count in each SSBO
uint selections_buffer;    // reference to graphics memory
uint ssbos[2];             // references to graphics memory
bool flip = true;          // to swap SSBO fastly

int fps_limit = 60;
float last_fps_estimate_result = 59;
float scale = 1;
vec2 anchor = vec2(-1, 0); // in use when shift pressed and right mouse pressed (change spawn_amount mode)
vec2 shift;
vec2 last_mouse_pos;
vec2 actual_mouse_pos;
vec2 selection_start;
bool active_selection = false;

bool play = true;

///  F I L E S Y S T E M   R E L A T E D  ///

string savepath = "save.bin";

void load_particles()
{
    ifstream fs(savepath, ios::in | ios::binary);

    if (!fs.is_open())
    {
        throw runtime_error("Cannot open file " + savepath);
    }

    fs.seekg(0, fs.end);
    uint length = fs.tellg();
    fs.seekg(0, fs.beg);

    char* data = new char[length];

    fs.read(data, length);
    if (!fs)
    {
        throw runtime_error("Error while reading from " + savepath);
    }
    else if (length % 16 != 0)
    {
        throw runtime_error("Incorrect file length, load terminated");
    }
    else
    {
        particles_count = length / 16;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[!flip]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, length, data, GL_DYNAMIC_COPY);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[flip]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, length, 0, GL_DYNAMIC_COPY);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, selections_buffer);

        uint* data = new uint[particles_count]();
        glBufferData(GL_SHADER_STORAGE_BUFFER, particles_count * sizeof(uint), data, GL_DYNAMIC_COPY);
        delete[] data;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    fs.close();
    delete[] data;

    notification_label.update(to_string(particles_count) + " particles was loaded");
}

string get_content_from_file(const string filepath)
{
    ifstream fs(filepath, ios::in | ios::binary);

    if (!fs.is_open())
    {
        throw runtime_error("Cannot open file " + filepath);
    }

    string content;

    fs.seekg(0, fs.end);
    content.reserve(fs.tellg());
    fs.seekg(0, fs.beg);

    content.assign((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

    fs.close();

    return content;
}

///  S H A D E R   R E L A T E D  ///

uint try_to_compile_shader(GLenum type, const string source)
{
    uint compiled_shader_id = glCreateShader(type);
    {
        const char* src = source.data();
        glShaderSource(compiled_shader_id, 1, &src, 0);
        glCompileShader(compiled_shader_id);
    }

    {
        int is_compiled;
        glGetShaderiv(compiled_shader_id, GL_COMPILE_STATUS, &is_compiled);

        if (is_compiled != GL_TRUE)
        {
            GLsizei log_length;
            glGetShaderiv(compiled_shader_id, GL_INFO_LOG_LENGTH, &log_length);

            string msg;
            msg.reserve(log_length);

            glGetShaderInfoLog(compiled_shader_id, log_length, &log_length, (char*)msg.data());

            glDeleteShader(compiled_shader_id);

            throw runtime_error(msg);
        }
        else
            return compiled_shader_id;
    }
}

void load_and_apply_vertex_shader(uint name)
{
    string path = "shaders/";
    path += char(name + 48);
    path += ".glsl";

    uint vertex_shader_id = try_to_compile_shader(GL_VERTEX_SHADER, get_content_from_file(path));

    if ((vertex_shader_id != 0) && (frag_shader_id != 0))
    {
        uint program_id = glCreateProgram();
        {
            glAttachShader(program_id, vertex_shader_id);
            glAttachShader(program_id, frag_shader_id);

            glLinkProgram(program_id);

            glDeleteShader(vertex_shader_id);
        }

        // link program
        {
            int is_linked;
            glGetProgramiv(program_id, GL_LINK_STATUS, &is_linked);

            if (!is_linked)
            {
                GLsizei log_length;
                glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

                string msg;
                msg.reserve(log_length);

                glGetProgramInfoLog(program_id, log_length, &log_length, (char*)msg.data());

                glDeleteProgram(program_id);

                throw runtime_error(msg);
            }
        }

        // enable program
        {
            if (cur_prog_id != 0)
            {
                glDeleteProgram(cur_prog_id);
            }
            cur_prog_id = program_id;
            glUseProgram(cur_prog_id);
        }

        // init uniforms
        {
            pass_uniform(cur_prog_id, "window_relatives", 870.f / vec2(window->getSize()));
            pass_uniform(cur_prog_id, "shift", shift);
            pass_uniform(cur_prog_id, "scale", scale);
            pass_uniform(cur_prog_id, "mouse_left_pressed", Mouse::isButtonPressed(Mouse::Left));
            pass_uniform(cur_prog_id, "mouse_right_pressed", Mouse::isButtonPressed(Mouse::Right));
            pass_uniform(cur_prog_id, "mouse_middle_pressed", Mouse::isButtonPressed(Mouse::Middle));
            pass_uniform(
                cur_prog_id,
                "mouse_pos",
                map_coord_from(*window, actual_mouse_pos, shift, scale)
            );
        }
    }

    notification_label.update("shader " + to_string(name) + ".glsl applied");
}

///  B U F F E R S   M A N A G M E N T  ///

void swap_particles_buffers()
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbos[flip]);
    flip = !flip;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbos[flip]);
}

void update_prepared_spawn()
{
    delete[] prepared_spawn;
    prepared_spawn = new vec2[spawn_amount];

    for (uint i = 0; i < spawn_amount; i++)
    {
        float angle = float(rand()) * 6.2831 / RAND_MAX;
        float dist = sqrt(float(rand()) / RAND_MAX);

        prepared_spawn[i] = vec2(
            cosf(angle) * dist,
            sinf(angle) * dist
        );
    }
}

///  E N T R Y  P O I N T  &  M A I N  L O O P  ///

int main()
{
    // init //
    window = new RenderWindow(VideoMode(870, 870), "P A R T I C L E S");
    window->setFramerateLimit(0);
    {
        if (glewInit() != GLEW_OK)
        {
            return -1;
        }

        // prepare shaders
        {
            frag_shader_id = try_to_compile_shader(
                GL_FRAGMENT_SHADER,
                "#version 330 core\nin vec4 pixel_color;\nout vec4 color;\nvoid main(){color = vec4(pixel_color);}"
            );
            load_and_apply_vertex_shader(1);
        }

        // buffers init
        {
            glGenBuffers(2, ssbos);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbos[0]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbos[1]);

            glGenBuffers(1, &selections_buffer);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, selections_buffer);
            uint* data = new uint[particles_count]();
            glBufferData(GL_SHADER_STORAGE_BUFFER, particles_count * sizeof(uint), data, GL_DYNAMIC_COPY);
            delete[] data;
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, selections_buffer);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }

        // prepare open gl
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glPolygonMode(GL_FRONT, GL_FILL);
            glEnable(GL_PROGRAM_POINT_SIZE);
        }

        load_particles();

        update_prepared_spawn();

        default_font.loadFromFile("font.ttf");
    }

    Clock clock;
    Clock last_draw;
    Clock last_fps_estimate;

    // main loop //
    while (window->isOpen())
    {
        // calculate new actual mouse position
        actual_mouse_pos = vec2(Mouse::getPosition(*window));
        actual_mouse_pos.y = window->getSize().y - actual_mouse_pos.y;

        // converted vectors //

        // mouse
        vec2 mp = map_coord_to(*window, actual_mouse_pos);
        vec2 dp = map_coord_from(*window, actual_mouse_pos, shift, scale);

        // selection
        vec2 ss;
        vec2 ds;

        // handle input //
        try
        {
            bool mouse_moved = false;

            // process events (mouse/key/scroll/etc...)
            Event event;
            while (window->pollEvent(event))
            {
                if (event.type == Event::MouseMoved)
                {
                    mouse_moved = true;
                    vec2 mouse_pos = vec2(event.mouseMove.x, window->getSize().y - event.mouseMove.y);

                    if (!(Mouse::isButtonPressed(Mouse::Right) && (Keyboard::isKeyPressed(Keyboard::RShift) || Keyboard::isKeyPressed(Keyboard::LShift))))
                    {
                        if (Mouse::isButtonPressed(Mouse::Left) && !(Keyboard::isKeyPressed(Keyboard::RShift) || Keyboard::isKeyPressed(Keyboard::LShift)))
                        {
                            glBindBuffer(GL_SHADER_STORAGE_BUFFER, selections_buffer);

                            vector<uint> selected;

                            // retrive selected
                            {
                                uint* data = new uint[particles_count];
                                glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, particles_count * sizeof(uint), data);
                                for (uint i = 0; i < particles_count; i++)
                                {
                                    if (data[i])
                                    {
                                        selected.push_back(i);
                                    }
                                }
                                delete[] data;
                            }

                            if (selected.empty())
                            {
                                // drag field
                                shift.x += (mouse_pos.x - last_mouse_pos.x) * scale;
                                shift.y += (mouse_pos.y - last_mouse_pos.y) * scale;
                                pass_uniform(cur_prog_id, "shift", shift);
                            }
                            else
                            {
                                // drag selected
                                particle* data = new particle[particles_count];
                                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[!flip]);
                                glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, particles_count * sizeof(particle), data);
                                for (uint const& i : selected)
                                {
                                    data[i].x += (mouse_pos.x - last_mouse_pos.x) * scale;
                                    data[i].y += (mouse_pos.y - last_mouse_pos.y) * scale;
                                }
                                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, particles_count * sizeof(particle), data);
                                delete[] data;
                            }

                            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
                        }

                        pass_uniform(
                            cur_prog_id,
                            "mouse_pos",
                            map_coord_from(*window, mouse_pos, shift, scale)
                        );
                    }

                    last_mouse_pos = mouse_pos;
                }
                else if (event.type == Event::KeyPressed)
                {
                    if (event.key.code == Keyboard::Space)
                    {
                        play = !play;
                        notification_label.update(play ? "play" : "pause");
                    }
                    else if (event.key.code == Keyboard::Delete)
                    {
                        uint* selections = new uint[particles_count];

                        glBindBuffer(GL_SHADER_STORAGE_BUFFER, selections_buffer);
                        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, particles_count * sizeof(uint), selections);

                        uint new_particles_count = 0;
                        for (uint i = 0; i < particles_count; i++)
                        {
                            if (!selections[i])
                                new_particles_count++;
                        }

                        uint* zdata = new uint[new_particles_count]();
                        glBufferData(GL_SHADER_STORAGE_BUFFER, new_particles_count * sizeof(uint), zdata, GL_DYNAMIC_COPY);
                        delete[] zdata;

                        particle* data = new particle[particles_count];
                        particle* new_data = new particle[new_particles_count];
                        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[!flip]);
                        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, particles_count * sizeof(particle), data);

                        uint j = 0;
                        for (uint i = 0; i < particles_count; i++)
                        {
                            if (!selections[i])
                            {
                                new_data[j] = data[i];
                                j++;
                            }
                        }

                        glBufferData(GL_SHADER_STORAGE_BUFFER, new_particles_count * sizeof(particle), new_data, GL_DYNAMIC_COPY);
                        delete[] data;
                        delete[] new_data;

                        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[flip]);
                        glBufferData(GL_SHADER_STORAGE_BUFFER, new_particles_count * sizeof(particle), 0, GL_DYNAMIC_COPY);

                        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

                        delete[] selections;

                        string str;

                        str += to_string(particles_count - new_particles_count) + " particle";
                        str += particles_count - new_particles_count == 1 ? "" : "s";
                        str += " was deleted";

                        notification_label.update(str);

                        particles_count = new_particles_count;
                    }
                    else if ((event.key.code >= Keyboard::Num0) && (event.key.code <= Keyboard::Num9))
                    {
                        load_and_apply_vertex_shader(event.key.code - Keyboard::Num0);
                    }
                    else if ((event.key.code == Keyboard::Up) || (event.key.code == Keyboard::Down))
                    {
                        fps_limit += 16 * ((event.key.code == Keyboard::Up) * 2 - 1);
                        if (fps_limit < 3)
                            fps_limit = 3;

                        fps_label.update(last_fps_estimate_result, fps_limit);
                    }
                    else if (Keyboard::isKeyPressed(Keyboard::RControl) || Keyboard::isKeyPressed(Keyboard::LControl))
                    {
                        if (event.key.code == Keyboard::S)
                        {
                            ofstream fs(savepath, ios::out | ios::binary | ios::trunc);

                            if (!fs.is_open())
                            {
                                throw runtime_error("Cannot open file " + savepath);
                            }

                            glBindBuffer(GL_ARRAY_BUFFER, ssbos[!flip]);

                            char* data = new char[particles_count * sizeof(particle)];

                            glGetBufferSubData(GL_ARRAY_BUFFER, 0, particles_count * sizeof(particle), data);
                            glBindBuffer(GL_ARRAY_BUFFER, 0);

                            fs.write(data, particles_count * sizeof(particle));
                            if (!fs)
                            {
                                throw runtime_error("Error while writing to " + savepath);
                            }
                            fs.close();

                            delete[] data;

                            notification_label.update("particles state was saved");
                        }
                        else if (event.key.code == Keyboard::L)
                        {
                            load_particles();
                        }
                        else if (event.key.code == Keyboard::C)
                        {
                            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[0]);
                            glBufferData(GL_SHADER_STORAGE_BUFFER, 0, 0, GL_DYNAMIC_COPY);

                            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[1]);
                            glBufferData(GL_SHADER_STORAGE_BUFFER, 0, 0, GL_DYNAMIC_COPY);

                            glBindBuffer(GL_SHADER_STORAGE_BUFFER, selections_buffer);
                            glBufferData(GL_SHADER_STORAGE_BUFFER, 0, 0, GL_DYNAMIC_COPY);

                            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

                            particles_count = 0;

                            notification_label.update("cleared");
                        }
                    }
                }
                else if (event.type == Event::Resized)
                {
                    vec2 dimensions = vec2(event.size.width, event.size.height);
                    pass_uniform(cur_prog_id, "window_relatives", 870.f / dimensions);
                    glViewport(0, 0, dimensions.x, dimensions.y);
                    window->setView(View(FloatRect(0, 0, dimensions.x, dimensions.y)));

                    pass_uniform(
                        cur_prog_id,
                        "mouse_pos",
                        map_coord_from(*window, actual_mouse_pos, shift, scale)
                    );
                }
                else if (event.type == Event::MouseWheelScrolled)
                {
                    float k = pow(1.12, -event.mouseWheelScroll.delta);

                    if (Keyboard::isKeyPressed(Keyboard::RShift) || Keyboard::isKeyPressed(Keyboard::LShift))
                    {
                        spawn_radius *= k;
                    }
                    else
                    {
                        vec2 dimensions = vec2(window->getSize()) / 2.f;

                        shift.x += scale * (dimensions.x - actual_mouse_pos.x) * (1 - k);
                        shift.y += scale * (dimensions.y - actual_mouse_pos.y) * (1 - k);

                        scale *= k;

                        pass_uniform(cur_prog_id, "scale", scale);
                        pass_uniform(cur_prog_id, "shift", shift);
                        pass_uniform(
                            cur_prog_id,
                            "mouse_pos",
                            map_coord_from(*window, actual_mouse_pos, shift, scale)
                        );
                    }
                }
                else if (event.type == Event::Closed)
                {
                    window->close();
                }
            }

            // selection
            if (!(Keyboard::isKeyPressed(Keyboard::RShift) || Keyboard::isKeyPressed(Keyboard::LShift)) && Mouse::isButtonPressed(Mouse::Right))
            {
                if (!active_selection)
                {
                    selection_start = actual_mouse_pos;
                    active_selection = true;
                    pass_uniform(cur_prog_id, "active_selection", active_selection);
                    pass_uniform(cur_prog_id, "selection", vec4(
                        dp.x, dp.y, dp.x, dp.y
                    ));
                }
            }
            else if (active_selection)
            {
                active_selection = false;
                pass_uniform(cur_prog_id, "active_selection", active_selection);

                uint* selections = new uint[particles_count];

                glBindBuffer(GL_SHADER_STORAGE_BUFFER, selections_buffer);
                glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, particles_count * sizeof(uint), selections);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

                uint selected_count = 0;
                for (uint i = 0; i < particles_count; i++)
                {
                    if (selections[i])
                        selected_count++;
                }

                delete[] selections;

                notification_label.update(to_string(selected_count) + " particle" + (selected_count == 1 ? "": "s" ) + " was selected");
            }

            // define converted selecion coordinates
            ss = map_coord_to(*window, selection_start);
            ds = map_coord_from(*window, selection_start, shift, scale);

            // pass selection to shader
            if (mouse_moved && active_selection)
            {
                vec4 sel(ds.x, ds.y, dp.x, dp.y);

                if (sel.z < sel.x)
                {
                    float tmp = sel.z;
                    sel.z = sel.x;
                    sel.x = tmp;
                }

                if (sel.w < sel.y)
                {
                    float tmp = sel.w;
                    sel.w = sel.y;
                    sel.y = tmp;
                }

                pass_uniform(cur_prog_id, "selection", sel);
            }

            // change spawn properties
            if (Mouse::isButtonPressed(Mouse::Right) && (Keyboard::isKeyPressed(Keyboard::RShift) || Keyboard::isKeyPressed(Keyboard::LShift)))
            {
                if (anchor.x == -1)
                {
                    anchor = actual_mouse_pos;
                    window->setMouseCursorVisible(false);
                }

                float d = actual_mouse_pos.x - anchor.x;

                Mouse::setPosition(Vector2i(anchor.x, window->getSize().y - anchor.y), *window);

                if (d != 0)
                {
                    if (spawn_amount + d < 1)
                        d = 1 - spawn_amount;

                    spawn_amount += d;

                    // update prepared spawn
                    vec2* new_prepared_spawn = new vec2[spawn_amount];
                    if (d < 0)
                    {
                        // cut prepared spawn
                        for (uint i = 0; i < spawn_amount; i++)
                            new_prepared_spawn[i] = prepared_spawn[i];
                        delete[] prepared_spawn;
                        prepared_spawn = new_prepared_spawn;
                    }
                    else
                    {
                        // copy whole prepared spawn
                        for (uint i = 0; i < spawn_amount - d; i++)
                            new_prepared_spawn[i] = prepared_spawn[i];
                        delete[] prepared_spawn;

                        // add new elements
                        for (uint i = spawn_amount - d; i < spawn_amount; i++)
                        {
                            float angle = float(rand()) * 6.2831 / RAND_MAX;
                            float dist = sqrt(float(rand()) / RAND_MAX);

                            new_prepared_spawn[i] = vec2(
                                cosf(angle) * dist,
                                sinf(angle) * dist
                            );
                        }

                        prepared_spawn = new_prepared_spawn;
                    }
                }
            }
            else if (anchor.x != -1)
            {
                window->setMouseCursorVisible(true);
                anchor.x = -1;
            }

            // throw particles
            if (Mouse::isButtonPressed(Mouse::Left) && (Keyboard::isKeyPressed(Keyboard::RShift) || Keyboard::isKeyPressed(Keyboard::LShift)))
            {
                // create particles
                particle* new_particles = new particle[spawn_amount];

                for (uint i = 0; i < spawn_amount; i++)
                {
                    new_particles[i] = vec4(
                        dp.x + prepared_spawn[i].x * spawn_radius,
                        dp.y + prepared_spawn[i].y * spawn_radius,
                        0,
                        0
                    );
                }

                update_prepared_spawn();

                // update capacity of the both buffers and
                // fill not up-to-date buffer with information from another buffer and add new particles in the end
                {
                    glBindBuffer(GL_COPY_WRITE_BUFFER, ssbos[flip]);
                    glBindBuffer(GL_COPY_READ_BUFFER, ssbos[!flip]);

                    glBufferData(GL_COPY_WRITE_BUFFER, (particles_count + spawn_amount) * sizeof(particle), 0, GL_DYNAMIC_COPY);
                    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, particles_count * sizeof(particle));
                    glBufferSubData(GL_COPY_WRITE_BUFFER, particles_count * sizeof(particle), spawn_amount * sizeof(particle), new_particles);
                    glBufferData(GL_COPY_READ_BUFFER, (particles_count + spawn_amount) * sizeof(particle), 0, GL_DYNAMIC_COPY);

                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, selections_buffer);
                    uint* data = new uint[particles_count + spawn_amount]();
                    glBufferData(GL_SHADER_STORAGE_BUFFER, (particles_count + spawn_amount) * sizeof(uint), data, GL_DYNAMIC_COPY);
                    delete[] data;

                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
                    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
                    glBindBuffer(GL_COPY_READ_BUFFER, 0);
                }

                delete[] new_particles;

                // so, previously up-to-date buffer currently is not up-to-date
                swap_particles_buffers();

                particles_count += spawn_amount;
            }
        }
        catch (runtime_error err)
        {
            size_t size = strlen(err.what()) + 1;
            wchar_t* msg = new wchar_t[size];

            size_t outSize;
            mbstowcs_s(&outSize, msg, size, err.what(), size - 1);

            MessageBox(
                window->getSystemHandle(),
                msg,
                L"Error",
                MB_OK
            );

            delete[] msg;
        }

        // render new frame //
        window->clear();
        {
            // render particles
            {
                pass_uniform(cur_prog_id, "time", clock.getElapsedTime().asSeconds());
                pass_uniform(cur_prog_id, "takt", takt);

                glClear(GL_COLOR_BUFFER_BIT);
                glDrawArrays(GL_POINTS, 0, particles_count);

                if (last_draw.getElapsedTime().asMilliseconds() * fps_limit >= 1000)
                {
                    if (play)
                    {
                        swap_particles_buffers();
                        takt++;
                    }

                    if (last_fps_estimate.getElapsedTime().asMilliseconds() > 500)
                    {
                        last_fps_estimate_result = 1000.0 / last_draw.getElapsedTime().asMilliseconds();
                        fps_label.update(last_fps_estimate_result, fps_limit);

                        last_fps_estimate.restart(); // set current time as the latest
                    }

                    last_draw.restart(); // set current time as the latest
                }
            }

            // fallback to manual operation
            glUseProgram(0);
            {
                if (Keyboard::isKeyPressed(Keyboard::RShift) || Keyboard::isKeyPressed(Keyboard::LShift))
                {
                    vec2 center;
                    if (anchor.x == -1)
                    {
                        center = actual_mouse_pos;
                    }
                    else
                    {
                        center = anchor;
                    }

                    // render spawn circle
                    draw_circle(*window, center, spawn_radius / scale, vec4(1, 1, 1, 1));

                    // render density representation
                    glBegin(GL_POINTS);
                    glColor4f(1, 1, 1, 1);
                    for (uint i = 0; i < spawn_amount; i++)
                    {
                        vec2 rd = map_coord_to(*window, center + prepared_spawn[i] * spawn_radius / scale);
                        glVertex2f(rd.x, rd.y);
                    }
                    glEnd();
                }

                // render selection box
                if (active_selection)
                {
                    glBegin(GL_LINE_LOOP);
                    {
                        glColor4f(0, 0.8, 1, 0.6);
                        glVertex2f(ss.x, ss.y);
                        glVertex2f(ss.x, mp.y);
                        glVertex2f(mp.x, mp.y);
                        glVertex2f(mp.x, ss.y);
                    }
                    glEnd();

                    glBegin(GL_QUADS);
                    {
                        glColor4f(0, 0.8, 1, 0.05);
                        glVertex2f(ss.x, ss.y);
                        glVertex2f(ss.x, mp.y);
                        glVertex2f(mp.x, mp.y);
                        glVertex2f(mp.x, ss.y);
                    }
                    glEnd();
                }
            }

            window->pushGLStates();
            fps_label.render(*window);
            notification_label.render(*window);
            window->popGLStates();

            glUseProgram(cur_prog_id);
        }
        window->display();
    }

    glDeleteBuffers(2, ssbos);
    glDeleteBuffers(1, &selections_buffer);
    glDeleteProgram(cur_prog_id);
    glDeleteShader(frag_shader_id);

    return 0;
}