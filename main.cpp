#include <cstdlib>
#include <iostream>
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "bezier.h"

using namespace std;

int main( int argc, char* argv[] )
{
	/* Variables */
	bool gameRunning = true, clicked = false;
	int downMouseX, downMouseY;
	int lineIndex, pointIndex;
	SDL_Event event;			// dump event polls into this


	/* Initialize SDL */
	if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )	// should use SDL_INIT_VIDEO instead to save execution time
	{
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
	}
    atexit(SDL_Quit);	// push SDL_Quit onto stack to be executed at program end

	/* Initialize SDL window */
	const SDL_VideoInfo* myPointer = SDL_GetVideoInfo();	// get current display information (for height, width, color depth, etc.)
	// set new window to half current screen size, as a hardware surface, enable double buffering[, without a frame]
	SDL_Surface *screen = SDL_SetVideoMode( myPointer->current_w/2, myPointer->current_h/2, 0, SDL_HWSURFACE|SDL_DOUBLEBUF );		/* End SDL initialization */

	/* Set window icon and title */
	Uint32 colorkey;
	SDL_Surface *icon;
	icon = SDL_LoadBMP("icon.bmp");
	colorkey = SDL_MapRGB( icon->format, 255,255,255 );
	SDL_SetColorKey( icon, SDL_SRCCOLORKEY, colorkey );
	SDL_WM_SetIcon( icon, NULL );
	SDL_FreeSurface( icon );
    SDL_WM_SetCaption( "SDL sandbox application", "Minimized" );
	
	/* Game loop */
	Bezier curves(screen);
	while( gameRunning )
	{
		while( SDL_PollEvent(&event) )
		{
			switch( event.type )
			{
				case SDL_ACTIVEEVENT:		// see http://www.libsdl.org/cgi/docwiki.cgi/SDL_ActiveEvent
					break;
				case SDL_KEYDOWN:			// keyboard pressed
					break;
				case SDL_KEYUP:				// keyboard released
					if( event.key.keysym.sym == SDLK_ESCAPE )
						gameRunning = false;
					break;
				case SDL_MOUSEBUTTONDOWN:	// mouse pressed
					downMouseX = event.button.x;	// get mouse click location
					downMouseY = event.button.y;	//
					break;
				case SDL_MOUSEBUTTONUP:		// mouse released
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						curves.addLine();
						if( curves.select( downMouseX, downMouseY ) )
							circleRGBA( screen, downMouseX, downMouseY, 5, 255,255,255,255 );
						if( clicked )	// dragging a point
						{
						}
						else
							clicked = !clicked;		// toggle mouse state
					}
					else if( event.button.button == SDL_BUTTON_RIGHT )
					{
						curves.drawLines();
					}
					break;
				case SDL_MOUSEMOTION:		// mouse moved
					if( clicked )	// dragging a point
					{
//						curves.getClosest( event.motion.x, event.motion.y, lineIndex, pointIndex );
					}
					else
					{
					}
					break;
				case SDL_QUIT:				// top-right X clicked
					gameRunning = false;
					break;
				default:
					break;
			}
		}
	}

//	system("PAUSE");
	return EXIT_SUCCESS;
}
