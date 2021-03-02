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

void draw_circle(vec2 center, float r, int num_segments, vec4 color)
{
	float theta = 2 * 3.1415926 / float(num_segments);
	float c = cosf(theta); //precalculate the sine and cosine
	float s = sinf(theta);
	float t;

	float x = r; //we start at angle = 0 
	float y = 0;

	glBegin(GL_LINE_LOOP);
	glColor4f(color.x, color.y, color.z, color.w);
	for (int ii = 0; ii < num_segments; ii++)
	{
		glVertex2f(
			map_coord_to(x + center.x, WINDOW_WIDTH),
			map_coord_to(y + center.y, WINDOW_HEIGHT)
		);

		//apply the rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	}
	glEnd();
}