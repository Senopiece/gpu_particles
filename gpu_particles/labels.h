#pragma once

#include <SFML/Graphics.hpp>
#include <string>

using namespace std;
using namespace sf;

class FPS_Label
{
public:
    FPS_Label(Font& font);
    ~FPS_Label();

    void update(int fps_estimate_result, int fps_limit);
    void render(RenderWindow& window);

private:
    Text* label;
};

class NotificationLabel
{
public:
    NotificationLabel(Font& font);
    ~NotificationLabel();

    void update(string new_text);
    void render(RenderWindow &window);

private:
    Text* label;
    Clock* fade_timer;
};