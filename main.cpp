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
	int xMouse, yMouse, downX, downY;
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
	curves.drawLines();
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
					else if( event.key.keysym.sym == SDLK_RETURN )
					{
						curves.addLine(1);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:	// mouse pressed
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						downX = event.button.x;	// get mouse click location
						downY = event.button.y;	//
						if( !curves.active )	// not in point-moving mode
						{
							if( curves.select( downX, downY ) )	// try to get a point (select if under mouse pointer)
							{
								curves.active = true;
								curves.move( downX, downY );
								curves.drawLines();
								// SDL_LockSurface( screen );
									// circleRGBA( screen, downX, downY, 5, 255,255,255,255 );
								// SDL_UnlockSurface( screen );
								// SDL_Flip( screen );
							}
						}
						else	// already moving a point
						{
							curves.active = false;		// get out of point-moving mode
						}
					}
					else if( event.button.button == SDL_BUTTON_RIGHT )
					{
						curves.addLine();
						curves.drawLines();
					}
					break;
				case SDL_MOUSEBUTTONUP:		// mouse released
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						if( (event.button.x - downX)*(event.button.x - downX) + (event.button.y - downY)*(event.button.y - downY) > 16 )	// mouse moved more than 4 pixels before unclicking
						{
							curves.active = false;
						}
					}
					break;
				case SDL_MOUSEMOTION:		// mouse moved
					if( curves.active )		// moving a point
					{
						xMouse = event.button.x;	// get mouse click location
						yMouse = event.button.y;	//
						curves.move( xMouse, yMouse );
						curves.drawLines();
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
