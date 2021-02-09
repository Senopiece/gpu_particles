#pragma once

float map_coord_from(float x, float shift, float dimension)
{
    return (x - dimension * 0.5f) * scale + 435 - shift;
}

float map_coord_to(float component, float m)
{
    m = m * 0.5f;
    return (component - m) / m;
}