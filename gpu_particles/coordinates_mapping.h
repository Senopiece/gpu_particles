#pragma once

#define SFML_STATIC

#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector4.hpp>

using namespace sf;

typedef Vector2f vec2;
typedef Vector4f vec4;

float map_coord_from(float x, float shift, float dimension, float scale);
vec2 map_coord_from(Window& window, vec2 v, vec2 shift, float scale);
float map_coord_to(float component, float m);
vec2 map_coord_to(Window& window, vec2 components);