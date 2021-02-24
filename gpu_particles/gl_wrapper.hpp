#pragma once

void pass_uniform(string name, float value)
{
    glProgramUniform1f(cur_prog_id, glGetUniformLocation(cur_prog_id, name.c_str()), value);
}

void pass_uniform(string name, uint value)
{
    glProgramUniform1ui(cur_prog_id, glGetUniformLocation(cur_prog_id, name.c_str()), value);
}

void pass_uniform(string name, bool value)
{
    glProgramUniform1ui(cur_prog_id, glGetUniformLocation(cur_prog_id, name.c_str()), value);
}

void pass_uniform(string name, vec2 v)
{
    glProgramUniform2f(cur_prog_id, glGetUniformLocation(cur_prog_id, name.c_str()), v.x, v.y);
}

void pass_uniform(string name, vec4 v)
{
    glProgramUniform4f(cur_prog_id, glGetUniformLocation(cur_prog_id, name.c_str()), v.x, v.y, v.z, v.w);
}