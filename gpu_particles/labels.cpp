#include "labels.h"

// FPS_Label //

FPS_Label::FPS_Label(Font& font)
{
	label = new Text("fps: estimating...", font);
}

FPS_Label::~FPS_Label()
{
	delete label;
}

void FPS_Label::update(int fps_estimate_result, int fps_limit)
{
	label->setString("fps: " + to_string(fps_estimate_result) + " / " + to_string(fps_limit));
}

void FPS_Label::render(RenderWindow& window)
{
	window.draw(*label);
}

// NotificationLabel //

NotificationLabel::NotificationLabel(Font& font)
{
	label = new Text(">", font, 25);
	label->setPosition(0, 35);
	fade_timer = new Clock();
}

NotificationLabel::~NotificationLabel()
{
	delete label;
	delete fade_timer;
}

void NotificationLabel::update(string new_text)
{
	label->setString(">" + new_text);
	fade_timer->restart();
}

void NotificationLabel::render(RenderWindow& window)
{
	label->setFillColor(
		Color(255, 255, 255, max(0.f, 255 - fade_timer->getElapsedTime().asSeconds() * 50))
	);
	window.draw(*label);
}
