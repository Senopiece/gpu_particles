﻿#define GLEW_STATIC
#include <GL/glew.h>
#include <glfw3.h>
#include <iostream>

typedef unsigned int uint;
using namespace std;

#include "structures.hpp"

typedef vec4 particle;

GLFWwindow* window;

const string savepath = "save.bin";

uint particles_count = 0;

uint compiled_fragment_shader_id;
uint cur_prog_id = 0;

float WINDOW_WIDTH = 870;
float WINDOW_HEIGHT = 870;

uint selections_buffer;
uint ssbos[2];
bool flip = true;

float fps_limit = 60;
float scale = 1;
vec2 shift;
vec2 prev_mouse_pos;
vec2 selection_start;
bool active_selection = false;

bool left_ctrl_pressed = false;
bool right_ctrl_pressed = false;
bool play = true;

bool mouse_left_pressed = false;
bool mouse_right_pressed = false;
bool mouse_middle_pressed = false;

bool left_shift_pressed = false;
bool right_shift_pressed = false;

uint spawn_amount = 100;
float spawn_radius = 40;

#include "conversations.hpp"
#include "gl_wrapper.hpp"
#include "functools.hpp"

uint try_to_compile_shader(GLenum type, const string source)
{
    uint compiled_shader_id = glCreateShader(type);
    {
        const char* src = source.c_str();
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

            char* msg = new char[log_length];
            glGetShaderInfoLog(compiled_shader_id, log_length, &log_length, msg);

            cout << msg;

            delete[] msg;
            glDeleteShader(compiled_shader_id);

            return 0;
        }
        else
            return compiled_shader_id;
    }
}

void load_and_apply_shader(uint name)
{
    string path = "shaders/";
    path += char(name + 48);
    uint compiled_vertex_shader_id = try_to_compile_shader(GL_VERTEX_SHADER, get_content_from_file(path + ".glsl"));

    if ((compiled_vertex_shader_id != 0) && (compiled_fragment_shader_id != 0))
    {
        uint program_id = glCreateProgram();
        {
            glAttachShader(program_id, compiled_vertex_shader_id);
            glAttachShader(program_id, compiled_fragment_shader_id);

            glLinkProgram(program_id);

            glDeleteShader(compiled_vertex_shader_id);
        }

        // link program
        {
            int is_linked;
            glGetProgramiv(program_id, GL_LINK_STATUS, &is_linked);

            if (!is_linked)
            {
                GLsizei log_length;
                glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

                char* msg = new char[log_length];
                glGetProgramInfoLog(program_id, log_length, &log_length, msg);

                cout << msg;

                delete[] msg;
                glDeleteProgram(program_id);

                return;
            }
        }

        // enable program
        {
            if (cur_prog_id != 0)
                glDeleteProgram(cur_prog_id);
            cur_prog_id = program_id;
            glUseProgram(cur_prog_id);
        }

        // init uniforms
        {
            pass_uniform("window_relatives", vec2(870.f / WINDOW_WIDTH, 870.f / WINDOW_HEIGHT));
            pass_uniform("shift", shift);
            pass_uniform("scale", scale);
            pass_uniform("mouse_left_pressed", mouse_left_pressed);
            pass_uniform("mouse_right_pressed", mouse_right_pressed);
            pass_uniform("mouse_middle_pressed", mouse_middle_pressed);
            pass_uniform(
                "mouse_pos",
                vec2(
                    map_coord_from(prev_mouse_pos.x, shift.x, WINDOW_WIDTH),
                    map_coord_from(prev_mouse_pos.y, shift.y, WINDOW_HEIGHT)
                )
            );
        }

        cout << ">> Shader loaded successfully" << endl;
    }
}

void render()
{
    vec4 sel(
        map_coord_from(selection_start.x, shift.x, WINDOW_WIDTH),
        map_coord_from(selection_start.y, shift.y, WINDOW_HEIGHT),
        map_coord_from(prev_mouse_pos.x, shift.x, WINDOW_WIDTH),
        map_coord_from(prev_mouse_pos.y, shift.y, WINDOW_HEIGHT)
    );

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

    pass_uniform("selection", sel);
    pass_uniform("active_selection", active_selection);
    pass_uniform("time", (float)glfwGetTime());
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_POINTS, 0, particles_count);
}

void load_particles()
{
    ifstream fs(savepath, ios::in | ios::binary);

    if (!fs.is_open())
    {
        cout << ">> Cannot open file " + savepath << endl;
        return;
    }

    fs.seekg(0, fs.end);
    uint length = fs.tellg();
    fs.seekg(0, fs.beg);

    char* data = new char[length];

    fs.read(data, length);
    if (!fs)
        cout << ">> Error while reading from " + savepath << endl;
    else if (length % 16 != 0)
        cout << ">> Incorrect file length, load terminated" << endl;
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

        cout << ">> Particles loaded successfully, total: " << particles_count << endl;
    }

    fs.close();
    delete[] data;
}

void save_particles()
{
    ofstream fs(savepath, ios::out | ios::binary | ios::trunc);

    if (!fs.is_open())
    {
        cout << ">> Cannot open file " + savepath << endl;
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, ssbos[!flip]);

    char* data = new char[particles_count * sizeof(particle)];

    glGetBufferSubData(GL_ARRAY_BUFFER, 0, particles_count * sizeof(particle), data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    fs.write(data, particles_count * sizeof(particle));
    if (!fs)
        cout << ">> Error while writing to " + savepath << endl;
    fs.close();

    delete[] data;

    cout << ">> Particles saved successfully" << endl;
}

void spawn_particles()
{
    // create particles
    particle* new_particles = new particle[spawn_amount];

    float px = map_coord_from(prev_mouse_pos.x, shift.x, WINDOW_WIDTH);
    float py = map_coord_from(prev_mouse_pos.y, shift.y, WINDOW_HEIGHT);

    for (uint i = 0; i < spawn_amount; i += 1)
    {
        float angle = float(rand()) * 6.2831 / RAND_MAX;
        float dist = spawn_radius * sqrt(float(rand()) / RAND_MAX);

        new_particles[i] = vec4(
            px + cosf(angle) * dist,
            py + sinf(angle) * dist,
            0,
            0
        );
    }

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
    // flip buffer binding points
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbos[flip]);
        flip = !flip;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbos[flip]);
    }

    particles_count += spawn_amount;

    cout << ">> Created " << spawn_amount << " particle";
    cout << (spawn_amount == 1 ? "" : "s");
    cout << ", total: " << particles_count << endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_LEFT_CONTROL)
    {
        if (action == GLFW_PRESS)
            left_ctrl_pressed = true;
        else if (action == GLFW_RELEASE)
            left_ctrl_pressed = false;
    }
    else if (key == GLFW_KEY_RIGHT_CONTROL)
    {
        if (action == GLFW_PRESS)
            right_ctrl_pressed = true;
        else if (action == GLFW_RELEASE)
            right_ctrl_pressed = false;
    }
    else if (key == GLFW_KEY_LEFT_SHIFT)
    {
        if (action == GLFW_PRESS)
            left_shift_pressed = true;
        else if (action == GLFW_RELEASE)
            left_shift_pressed = false;
    }
    else if (key == GLFW_KEY_RIGHT_SHIFT)
    {
        if (action == GLFW_PRESS)
            right_shift_pressed = true;
        else if (action == GLFW_RELEASE)
            right_shift_pressed = false;
    }
    else if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_SPACE)
        {
            play = !play;
            cout << ">> " << (play ? "Play" : "Pause") << endl;
        }
        else if (key == GLFW_KEY_DELETE)
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
            
            cout << ">> ";
            cout << particles_count - new_particles_count << " particle";
            cout << (particles_count - new_particles_count == 1 ? "": "s");
            cout << " was deleted, still remains " << new_particles_count << " particle";
            cout << (new_particles_count == 1 ? "" : "s") << endl;

            particles_count = new_particles_count;
        }
        else if (key == GLFW_KEY_C)
        {
            if (left_ctrl_pressed || right_ctrl_pressed)
            {
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[0]);
                glBufferData(GL_SHADER_STORAGE_BUFFER, 0, 0, GL_DYNAMIC_COPY);

                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[1]);
                glBufferData(GL_SHADER_STORAGE_BUFFER, 0, 0, GL_DYNAMIC_COPY);

                glBindBuffer(GL_SHADER_STORAGE_BUFFER, selections_buffer);
                glBufferData(GL_SHADER_STORAGE_BUFFER, 0, 0, GL_DYNAMIC_COPY);

                glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

                particles_count = 0;

                cout << ">> Cleared" << endl;
            }
        }
        else if ((key >= GLFW_KEY_0) && (key <= GLFW_KEY_9))
        {
            load_and_apply_shader(key - GLFW_KEY_0);
        }
        else if ((key == GLFW_KEY_UP) || (key == GLFW_KEY_DOWN))
        {
            if (right_shift_pressed)
            {
                spawn_amount += 3 * ((key == GLFW_KEY_UP) * 2 - 1);
                if (spawn_amount <= 0)
                    spawn_amount = 1;
                cout << ">> Spawn amount: " << spawn_amount << endl;
            }
            else if (left_shift_pressed)
            {
                spawn_radius += 5 * ((key == GLFW_KEY_UP) * 2 - 1);
                if (spawn_radius <= 0)
                    spawn_radius = 0;
                cout << ">> Spawn radius: " << spawn_radius << endl;
            }
            else
            {
                fps_limit += 5 * ((key == GLFW_KEY_UP) * 2 - 1);
                if (fps_limit <= 0)
                    fps_limit = 1;
                cout << ">> FPS limit: " << fps_limit << endl;
            }
        }
        else if (left_ctrl_pressed || right_ctrl_pressed)
        {
            if (key == GLFW_KEY_S)
            {
                save_particles();
            }
            else if (key == GLFW_KEY_L)
            {
                load_particles();
            }
        }
    }
}

void window_resize_callback(GLFWwindow* window, int width, int height)
{
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;

    pass_uniform("window_relatives", vec2(870.0 / width, 870.0 / height));
    glViewport(0, 0, width, height);

    pass_uniform(
        "mouse_pos",
        vec2(
            map_coord_from(prev_mouse_pos.x, shift.x, WINDOW_WIDTH),
            map_coord_from(prev_mouse_pos.y, shift.y, WINDOW_HEIGHT)
        )
    );
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        mouse_left_pressed = (action == GLFW_PRESS);
        pass_uniform("mouse_left_pressed", mouse_left_pressed);
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        mouse_right_pressed = (action == GLFW_PRESS);
        pass_uniform("mouse_right_pressed", mouse_right_pressed);

        if (mouse_right_pressed)
        {
            spawn_particles();
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        mouse_middle_pressed = (action == GLFW_PRESS);
        pass_uniform("mouse_middle_pressed", mouse_middle_pressed);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    float mx = WINDOW_WIDTH * 0.5f;
    float my = WINDOW_HEIGHT * 0.5f;

    float k = pow(1.12, -yoffset);

    shift.x += scale * (mx - prev_mouse_pos.x) * (1 - k);
    shift.y += scale * (my - prev_mouse_pos.y) * (1 - k);

    scale *= k;

    pass_uniform("scale", scale);
    pass_uniform("shift", shift);
    pass_uniform(
        "mouse_pos",
        vec2(
            map_coord_from(prev_mouse_pos.x, shift.x, WINDOW_WIDTH),
            map_coord_from(prev_mouse_pos.y, shift.y, WINDOW_HEIGHT)
        )
    );
}

void cursor_position_change_callback(GLFWwindow* window, double xpos, double ypos)
{
    ypos = WINDOW_HEIGHT - ypos; // convert coordinate system

    // drag field
    if ((mouse_left_pressed) && !(left_shift_pressed || right_shift_pressed))
    {
        shift.x += (xpos - prev_mouse_pos.x)*scale;
        shift.y += (ypos - prev_mouse_pos.y)*scale;
        pass_uniform("shift", shift);
    }

    // throw particles
    if (mouse_right_pressed)
    {
        spawn_particles();
    }

    prev_mouse_pos.x = xpos;
    prev_mouse_pos.y = ypos;

    pass_uniform(
        "mouse_pos",
        vec2(
            map_coord_from(prev_mouse_pos.x, shift.x, WINDOW_WIDTH),
            map_coord_from(prev_mouse_pos.y, shift.y, WINDOW_HEIGHT)
        )
    );
}

void endwords()
{
    cout << endl << "Press any key to exit" << endl;
    cin.get();
}

int main()
{
    // Initialize window with open GL
    {
        cout << ">>> Welcome to the club buddy <<<" << endl;

        if (glfwInit())
        {
            cout << ">> GLFW initialized" << endl;
        }
        else
        {
            cout << ">> Cannot init GLFW" << endl << endl;
            endwords();
            return -1;
        }

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "particles", 0, 0);
        glfwSetWindowSizeCallback(window, window_resize_callback);
        glfwSetCursorPosCallback(window, cursor_position_change_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetScrollCallback(window, scroll_callback);

        double xtmp, ytmp;
        glfwGetCursorPos(window, &xtmp, &ytmp);
        prev_mouse_pos = vec2(xtmp, ytmp);

        if (window)
        {
            glfwMakeContextCurrent(window);
            glfwSetKeyCallback(window, key_callback);
            cout << ">> Window created" << endl;
        }
        else
        {
            cout << ">> Fail to create window" << endl << endl;
            endwords();
            return -1;
        }

        if (glewInit() == GLEW_OK)
        {
            cout << ">> GLEW initialized" << endl;
        }
        else
        {
            cout << ">> Cannot init GLEW" << endl << endl;
            endwords();
            return -1;
        }

        // prepare shaders
        {
            compiled_fragment_shader_id = try_to_compile_shader(
                GL_FRAGMENT_SHADER, 
                "#version 330 core\nin vec4 pixel_color;\nout vec4 color;\nvoid main(){color = vec4(pixel_color);}"
            );
            load_and_apply_shader(1);
        }

        // buffers init
        {
            glGenBuffers(2, ssbos);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbos[0]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbos[1]);

            glGenBuffers(1, &selections_buffer);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, selections_buffer);
            uint* data = new uint[particles_count]();
            glBufferData(GL_SHADER_STORAGE_BUFFER, particles_count * 4, data, GL_DYNAMIC_COPY);
            delete[] data;
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, selections_buffer);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }

        // load initial paricles
        {
            load_particles();
        }

        // prepare open gl
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glPolygonMode(GL_FRONT, GL_FILL);
        }
    }

    double prev_t = glfwGetTime();

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // render particles
        {
            render();

            if ((play) && (glfwGetTime() - prev_t >= 1.0 / fps_limit))
            {
                prev_t = glfwGetTime();
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbos[flip]);
                flip = !flip;
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbos[flip]);
            }
        }

        // render selection box
        glUseProgram(0);
        if (active_selection)
        {
            glBegin(GL_LINES);
            {
                glColor4f(0, 0.8, 1, 0.6);
                glVertex2f(
                    map_coord_to(selection_start.x, WINDOW_WIDTH),
                    map_coord_to(selection_start.y, WINDOW_HEIGHT)
                );
                glVertex2f(
                    map_coord_to(prev_mouse_pos.x, WINDOW_WIDTH),
                    map_coord_to(selection_start.y, WINDOW_HEIGHT)
                );

                glColor4f(0, 0.8, 1, 0.6);
                glVertex2f(
                    map_coord_to(selection_start.x, WINDOW_WIDTH),
                    map_coord_to(selection_start.y, WINDOW_HEIGHT)
                );
                glVertex2f(
                    map_coord_to(selection_start.x, WINDOW_WIDTH),
                    map_coord_to(prev_mouse_pos.y, WINDOW_HEIGHT)
                );

                glColor4f(0, 0.8, 1, 0.6);
                glVertex2f(
                    map_coord_to(prev_mouse_pos.x, WINDOW_WIDTH),
                    map_coord_to(prev_mouse_pos.y, WINDOW_HEIGHT)
                );
                glVertex2f(
                    map_coord_to(prev_mouse_pos.x, WINDOW_WIDTH),
                    map_coord_to(selection_start.y, WINDOW_HEIGHT)
                );

                glColor4f(0, 0.8, 1, 0.6);
                glVertex2f(
                    map_coord_to(prev_mouse_pos.x, WINDOW_WIDTH),
                    map_coord_to(prev_mouse_pos.y, WINDOW_HEIGHT)
                );
                glVertex2f(
                    map_coord_to(selection_start.x, WINDOW_WIDTH),
                    map_coord_to(prev_mouse_pos.y, WINDOW_HEIGHT)
                );
            }
            glEnd();

            glBegin(GL_QUADS);
            {
                glColor4f(0, 0.8, 1, 0.05);
                glVertex2f(
                    map_coord_to(selection_start.x, WINDOW_WIDTH),
                    map_coord_to(selection_start.y, WINDOW_HEIGHT)
                );
                glVertex2f(
                    map_coord_to(selection_start.x, WINDOW_WIDTH),
                    map_coord_to(prev_mouse_pos.y, WINDOW_HEIGHT)
                );
                glVertex2f(
                    map_coord_to(prev_mouse_pos.x, WINDOW_WIDTH),
                    map_coord_to(prev_mouse_pos.y, WINDOW_HEIGHT)
                );
                glVertex2f(
                    map_coord_to(prev_mouse_pos.x, WINDOW_WIDTH),
                    map_coord_to(selection_start.y, WINDOW_HEIGHT)
                );
            }
            glEnd();
        }
        glUseProgram(cur_prog_id);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // check for start selection
        if ((left_shift_pressed || right_shift_pressed) && (mouse_left_pressed))
        {
            if (!active_selection)
            {
                selection_start = prev_mouse_pos;
                active_selection = true;
            }
        }
        else
            active_selection = false;
    }

    glDeleteBuffers(2, ssbos);
    glDeleteBuffers(1, &selections_buffer);
    glDeleteProgram(cur_prog_id);
    glDeleteShader(compiled_fragment_shader_id);
    glfwTerminate();

    return 0;
}