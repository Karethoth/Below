#include "event.hh"

using std::string;

Event::Event( ) : type( UNDEF_EVENT ),
                  subType( UNDEF_SUB_EVENT )
{
	timer.Reset();
}


string EventTypeToStr( EVENT_TYPE_VAR type )
{
	string ret;

	switch( type )
	{
		case UNDEF_EVENT:      ret = "Undefined Event"; break;
		case NETWORK_EVENT:    ret = "Network Event"; break;
		case STATE_EVENT:      ret = "State Event"; break;
		case OBJECT_EVENT:     ret = "Object Event"; break;
		case SDL_INPUT_EVENT:  ret = "SDL Input Event"; break;
		case SDL_WINDOW_EVENT: ret = "SDL Window Event"; break;
		default: ret = "Unknown Event";
	}

	return ret;
}


string EventSubTypeToStr( EVENT_SUB_TYPE_VAR type )
{
	string ret;

	switch( type )
	{
		case UNDEF_SUB_EVENT: ret = "Undefined Sub Event"; break;

		case NETWORK_JOIN:    ret = "Network Join"; break;
		case NETWORK_PART:    ret = "Network Part"; break;
		case NETWORK_DATA_IN: ret = "Network Data In"; break;
		case NETWORK_PING:    ret = "Network Ping"; break;
		case NETWORK_PONG:    ret = "Network Pong"; break;

		case STATE_RUN_START: ret = "State Run Start"; break;
		case STATE_RUN_PAUSE: ret = "State Run Pause"; break;

		case OBJECT_CREATE:        ret = "Object Create"; break;
		case OBJECT_DESTROY:       ret = "Object Destroy"; break;
		case OBJECT_UPDATE:        ret = "Object Update"; break;
		case OBJECT_PARENT_ADD:    ret = "Object Parent Add"; break;
		case OBJECT_PARENT_REMOVE: ret = "Object Parent Remove"; break;
		case OBJECT_CHILD_ADD:     ret = "Object Child Add"; break;
		case OBJECT_CHILD_REMOVE:  ret = "Object Child Remove"; break;

		case SDL_MOUSE_DOWN:     ret = "SDL Mouse Down"; break;
		case SDL_MOUSE_UP:       ret = "SDL Mouse Up"; break;
		case SDL_MOUSE_MOVE:     ret = "SDL Mouse Move"; break;
		case SDL_MOUSE_WHEEL:    ret = "SDL Mouse Wheel"; break;
		case SDL_KEY_DOWN:       ret = "SDL Key Down"; break;
		case SDL_KEY_UP:         ret = "SDL Key Up"; break;
		case SDL_TEXT_INPUT:     ret = "SDL Text Input"; break;
		case SDL_TEXT_EDITING:   ret = "SDL Text Editing"; break;
		case SDL_JOYSTICK_INPUT: ret = "SDL Joystick Input"; break;

		case SDL_WINDOW_RESIZE:       ret = "SDL Window Resize"; break;
		case SDL_WINDOW_FOCUS_CHANGE: ret = "SDL Window Focus Change"; break;

		default: ret = "Unknown Sub Event";
	}

	return ret;
}

