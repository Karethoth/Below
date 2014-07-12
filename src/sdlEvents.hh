#include "events/event.hh"
#include <string>


struct SdlMouseButtonEvent : public Event
{
	SdlMouseButtonEvent()
	{
		type    = SDL_INPUT_EVENT;
		subType = UNDEF_SUB_EVENT;
	}


	SDL_MouseButtonEvent button;
};



struct SdlMouseMoveEvent : public Event
{
	SdlMouseMoveEvent()
	{
		type    = SDL_INPUT_EVENT;
		subType = SDL_MOUSE_MOVE;
	}


	SDL_MouseMotionEvent motion;
};



struct SdlMouseWheelEvent : public Event
{
	SdlMouseWheelEvent()
	{
		type    = SDL_INPUT_EVENT;
		subType = SDL_MOUSE_WHEEL;
	}


	SDL_MouseWheelEvent wheel;
};



struct SdlKeyDownEvent : public Event
{
	SdlKeyDownEvent()
	{
		type    = SDL_INPUT_EVENT;
		subType = SDL_KEY_DOWN;
	}


	SDL_KeyboardEvent key;
};



struct SdlKeyUpEvent : public Event
{
	SdlKeyUpEvent()
	{
		type    = SDL_INPUT_EVENT;
		subType = SDL_KEY_UP;
	}


	SDL_KeyboardEvent key;
};



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

