#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "sproutLine.h"

int main( int argc, char* argv[] )
{
	/* Variables */
	bool gameRunning = true, clicked = false, ended = false;
	int xMouse, yMouse, downX, downY, downRtX, downRtY;
	int lineIndex, pointIndex;
	SDL_Event event;	// dump event polls into this
	SDL_Rect *rect;		// for positioning stuff in the center of the screen

	/* Initialize SDL */
	if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )	// should use SDL_INIT_VIDEO instead to save execution time
	{
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
	}
    atexit(SDL_Quit);	// push SDL_Quit onto stack to be executed at program end

	/* Initialize SDL window */
	const SDL_VideoInfo* myPointer = SDL_GetVideoInfo();	// get current display information (for height, width, color depth, etc.)
	SDL_Surface *screen = SDL_SetVideoMode( myPointer->current_w*2/3, myPointer->current_h*2/3, 0, SDL_HWSURFACE|SDL_DOUBLEBUF );	// set new window to three-quarters current screen size, as a hardware surface, enable double buffering
	/* End SDL initialization */

	/* Set window icon and title */
	Uint32 colorkey;
	SDL_Surface *icon;
	icon = SDL_LoadBMP("icon.bmp");
	colorkey = SDL_MapRGB( icon->format, 255,255,255 );
	SDL_SetColorKey( icon, SDL_SRCCOLORKEY, colorkey );
	SDL_WM_SetIcon( icon, NULL );
	SDL_FreeSurface( icon );
    SDL_WM_SetCaption( "SDL sandbox application", "Minimized" );

	SDL_Surface *gameOver;
	gameOver = SDL_LoadBMP("gameOver.bmp");

	rect = new SDL_Rect;
	rect->x = screen->w/2 - gameOver->w/2;
	rect->y = screen->h/2 - gameOver->h/2;

	/* Game loop */
	Sprout sp(screen,3);	// create new Sprout object
	while( gameRunning )
	{
		while( SDL_PollEvent(&event) )
		{
			switch( event.type )
			{
				case SDL_QUIT:					// top-right X clicked
					gameRunning = false;
					break;
				case SDL_ACTIVEEVENT:	break;	// see http://www.libsdl.org/cgi/docwiki.cgi/SDL_ActiveEvent
				case SDL_KEYDOWN:		break;	// keyboard pressed
				case SDL_KEYUP:					// keyboard released
					if( event.key.keysym.sym == SDLK_ESCAPE )
						gameRunning = false;
					else if( event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE )
					{
						if( ended )
							return 0;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:	break;	// mouse pressed
				case SDL_MOUSEBUTTONUP:		// mouse released
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						if( ended )
							return 0;
						if( !sp.connect() )		// game has ended (all nodes have degree 3 or escape pressed)
						{
							ended = true;
							SDL_BlitSurface( gameOver, NULL, screen, rect );
							SDL_Flip( screen );
						}
					}
					break;
				case SDL_MOUSEMOTION:		// mouse moved
					sp.highlightNear(event.motion.x, event.motion.y);
					break;
				default:
					break;
			}
		}
	}


	// system("PAUSE");
	return 0;
}
