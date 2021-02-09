#pragma once

struct vec2 {
    float x;
    float y;

    vec2()
    {
        x = 0;
        y = 0;
    }

    vec2(float _x, float _y)
    {
        x = _x;
        y = _y;
    }
};

struct vec4 {
    float x;
    float y;
    float z;
    float w;

    vec4()
    {
        x = 0;
        y = 0;
        z = 0;
        w = 0;
    }

    vec4(float _x, float _y, float _z, float _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }
};