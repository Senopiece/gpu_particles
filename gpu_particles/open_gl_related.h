#pragma once

#include <GL/glew.h>
#include <string>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector4.hpp>
#include <SFML/Graphics.hpp>

#include "coordinates_mapping.h"

using namespace std;
using namespace sf;

typedef Vector2f vec2;
typedef Vector4f vec4;
typedef unsigned int uint;

void pass_uniform(uint prog_id, string name, float value);
void pass_uniform(uint prog_id, string name, uint value);
void pass_uniform(uint prog_id, string name, bool value);
void pass_uniform(uint prog_id, string name, vec2 v);
void pass_uniform(uint prog_id, string name, vec4 v);
void draw_circle(Window& window, vec2 center, float r, vec4 color);