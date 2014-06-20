#include "events/event.hh"
#include <string>


struct SdlTextInputEvent : public Event
{
	SdlTextInputEvent()
	{
		type    = SDL_INPUT_EVENT;
		subType = SDL_TEXT_INPUT;
	}


	std::string text;
};



struct SdlTextEditingEvent : public Event
{
	SdlTextEditingEvent()
	{
		type    = SDL_INPUT_EVENT;
		subType = SDL_TEXT_EDITING;
	}


	std::string composition;
	int         cursor;
	int         selectionLength;
};



struct SdlJoystickInputEvent : public Event
{
	SdlJoystickInputEvent()
	{
		type    = SDL_INPUT_EVENT;
		subType = SDL_JOYSTICK_INPUT;
	}


	int  id;
	bool button;
	int  value;
};



struct SdlWindowResizeEvent : public Event
{
	SdlWindowResizeEvent()
	{
		type    = SDL_WINDOW_EVENT;
		subType = SDL_WINDOW_RESIZE;
	}


	int width;
	int height;
};



struct SdlWindowFocusChangeEvent : public Event
{
	SdlWindowFocusChangeEvent()
	{
		type    = SDL_WINDOW_EVENT;
		subType = SDL_WINDOW_FOCUS_CHANGE;
	}


	bool focusGained;
};

