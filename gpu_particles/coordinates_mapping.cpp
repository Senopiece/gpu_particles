#include "coordinates_mapping.h"

float map_coord_from(float x, float shift, float dimension, float scale)
{
    return (x - dimension * 0.5f) * scale + 435 - shift;
}

vec2 map_coord_from(Window& window, vec2 v, vec2 shift, float scale)
{
    vec2 dimensions = vec2(window.getSize());
    return vec2(
        map_coord_from(v.x, shift.x, dimensions.x, scale),
        map_coord_from(v.y, shift.y, dimensions.y, scale)
    );
}

float map_coord_to(float component, float m)
{
    m = m * 0.5f;
    return (component - m) / m;
}

vec2 map_coord_to(Window& window, vec2 components)
{
    vec2 dimensions = vec2(window.getSize());
    return vec2(
        map_coord_to(components.x, dimensions.x),
        map_coord_to(components.y, dimensions.y)
    );
}