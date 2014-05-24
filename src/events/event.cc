#include "event.hh"

Event::Event( ) : type( UNDEF_EVENT ),
                  subType( UNDEF_SUB_EVENT )
{
	timer.Reset();
}

