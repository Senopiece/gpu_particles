#include "open_gl_related.h"

void pass_uniform(uint prog_id, string name, float value)
{
    glProgramUniform1f(prog_id, glGetUniformLocation(prog_id, name.c_str()), value);
}

void pass_uniform(uint prog_id, string name, uint value)
{
    glProgramUniform1ui(prog_id, glGetUniformLocation(prog_id, name.c_str()), value);
}

void pass_uniform(uint prog_id, string name, bool value)
{
    glProgramUniform1ui(prog_id, glGetUniformLocation(prog_id, name.c_str()), value);
}

void pass_uniform(uint prog_id, string name, vec2 v)
{
    glProgramUniform2f(prog_id, glGetUniformLocation(prog_id, name.c_str()), v.x, v.y);
}

void pass_uniform(uint prog_id, string name, vec4 v)
{
    glProgramUniform4f(prog_id, glGetUniformLocation(prog_id, name.c_str()), v.x, v.y, v.z, v.w);
}

void draw_circle(Window& window, vec2 center, float r, vec4 color)
{
    float theta = 2 * 3.1415926f / float(40);
    float c = cosf(theta); //precalculate the sine and cosine
    float s = sinf(theta);
    float t;

    float x = r; //we start at angle = 0 
    float y = 0;

    vec2 dimensions = vec2(window.getSize());

    glBegin(GL_LINE_LOOP);
    glColor4f(color.x, color.y, color.z, color.w);
    for (int ii = 0; ii < 40; ii++)
    {
        glVertex2f(
            map_coord_to(x + center.x, dimensions.x),
            map_coord_to(y + center.y, dimensions.y)
        );

        //apply the rotation matrix
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;
    }
    glEnd();
}
