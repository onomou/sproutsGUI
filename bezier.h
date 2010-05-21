#include <vector>
#include <cmath>
#include "xorRNG.h"
#include "draw.h"

class Bezier
{
	private:
		struct bLine
		{
			int *xPoints[4];	// array of 4 pointers to integers
			int *yPoints[4];	//   "...
			int activePoint;
		};
		std::vector<bLine> allLines;
		int activeLine;
		int r,g,b,a;
		SDL_Surface *surface;	// screen to draw onto
		SDL_Surface *picking;
	public:
		Bezier(SDL_Surface*);
		void addLine(void);		// randomly generate a line
		void addLine(int);		// make new line with user input
		void addLine(int,int,int,int,int,int,int,int);
		bool select(int,int);	// accepts x,y : if close to a point, set that point active
		void move(int,int);
		double dist(int,int,int,int);
		void drawLines(void);	// blank surface, then draw all lines in the structure
		bool active;			// true if moving a point
};
Bezier::Bezier(SDL_Surface *sf)
{
	/* Set drawing surface */
	surface = sf;
	picking = SDL_ConvertSurface(sf, sf->format, sf->flags);	// copy current video surface
	SDL_FillRect( picking, NULL, 0 );							// blank picking surface

	/* Generate a new curve */
	bLine tmpBezier;
	for( int i = 0; i < 4; i++ )
	{
		tmpBezier.xPoints[i] = new int;
		*tmpBezier.xPoints[i] = random(surface->w);
		tmpBezier.yPoints[i] = new int;
		*tmpBezier.yPoints[i] = random(surface->h);
	}
	tmpBezier.activePoint = 0;

	allLines.push_back(tmpBezier);
	activeLine = allLines.size() - 1;

	/* Set colors */
	r = g = b = a = 255;

	active = false;		// not moving a point initially
}
void Bezier::addLine(void)
{
	bLine tmpBezier;
	for( int i = 0; i < 4; i++ )
	{
		tmpBezier.xPoints[i] = new int;
		*tmpBezier.xPoints[i] = random(surface->w);
		tmpBezier.yPoints[i] = new int;
		*tmpBezier.yPoints[i] = random(surface->h);
	}
	tmpBezier.activePoint = 0;

	allLines.push_back(tmpBezier);
	activeLine = allLines.size() - 1;
}
void Bezier::addLine(int junk)
{
	bool run = true;
	int xMouse, yMouse, pointBackup;
	bLine tmpBezier;
	SDL_Event event;
	SDL_PollEvent(&event);
	for( int i = 0; i < 4; i++ )
	{
		tmpBezier.xPoints[i] = new int;
		tmpBezier.yPoints[i] = new int;
		*tmpBezier.xPoints[i] = surface->w/2;		// initialize all points to current mouse location
		*tmpBezier.yPoints[i] = surface->h/2;
	}
	tmpBezier.activePoint = 0;
	allLines.push_back(tmpBezier);
	activeLine = allLines.size() - 1;
	
	// Note to self: I have replaced allLines[activeLine] with allLines.back() since the new curve is pushed to the back of allLines - is this replacement more efficient
	while( allLines.back().activePoint < 4 && run )
	{
		while( SDL_PollEvent(&event) )
		{
			switch( event.type )
			{
				case SDL_KEYUP:				// keyboard released
					if( event.key.keysym.sym == SDLK_ESCAPE )
						run = false;
					break;
				case SDL_MOUSEBUTTONDOWN:	// mouse pressed
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						xMouse = event.button.x;	// get mouse click location
						yMouse = event.button.y;	//

						pointBackup = allLines.back().activePoint;		// backup active point in case select() changes it
						if( select(xMouse,yMouse) )		// clicked near another node : snap to that node	(WARNING! This function will change activeLine and activePoint if it returns true)
						{
							allLines.back().xPoints[allLines.back().activePoint] = allLines[activeLine].xPoints[allLines[activeLine].activePoint];
							allLines.back().yPoints[allLines.back().activePoint] = allLines[activeLine].yPoints[allLines[activeLine].activePoint];
							activeLine = allLines.size() - 1;
							allLines.back().activePoint = pointBackup;
						}
						else
						{
							*allLines.back().xPoints[allLines.back().activePoint] = xMouse;
							*allLines.back().yPoints[allLines.back().activePoint] = yMouse;
						}
						drawLines();
						allLines.back().activePoint++;
					}
					else if( event.button.button == SDL_BUTTON_RIGHT )
					{
						run = false;
					}
					break;
				case SDL_MOUSEMOTION:		// mouse moved
					xMouse = event.button.x;	// get mouse click location
					yMouse = event.button.y;	//
					*allLines.back().xPoints[allLines.back().activePoint] = xMouse;
					*allLines.back().yPoints[allLines.back().activePoint] = yMouse;
					drawLines();
					break;
				case SDL_QUIT:				// top-right X clicked
					run = false;
					break;
				default:
					break;
			}
		}
	}
	if( !run )		// broke out unsuccessfully
	{
		allLines.pop_back();
		drawLines();
	}
}
void Bezier::addLine(int ax, int ay, int bx, int by, int cx, int cy, int dx, int dy)
{
	bLine tmpBezier;

	tmpBezier.xPoints[1] = new int;
	tmpBezier.xPoints[2] = new int;
	tmpBezier.xPoints[3] = new int;
	tmpBezier.xPoints[4] = new int;

	tmpBezier.yPoints[1] = new int;
	tmpBezier.yPoints[2] = new int;
	tmpBezier.yPoints[3] = new int;
	tmpBezier.yPoints[4] = new int;

	*tmpBezier.xPoints[1] = ax;
	*tmpBezier.xPoints[2] = bx;
	*tmpBezier.xPoints[3] = cx;
	*tmpBezier.xPoints[4] = dx;

	*tmpBezier.yPoints[1] = ay;
	*tmpBezier.yPoints[2] = by;
	*tmpBezier.yPoints[3] = cy;
	*tmpBezier.yPoints[4] = dy;

	tmpBezier.activePoint = 0;

	allLines.push_back(tmpBezier);
	activeLine = allLines.size() - 1;
}
// http://www.libsdl.org/intro.en/usingvideo.html
bool Bezier::select(int x, int y)
{
	int radiusRadius = 49;	// square of radius
	//int xx = x * x, yy = y * y;		// squares of components
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		for( int i = 0; i < 4; i++ )
		{
			if( (*allLines[lineIterator].xPoints[i] - x) * (*allLines[lineIterator].xPoints[i] - x) + (*allLines[lineIterator].yPoints[i] - y) * (*allLines[lineIterator].yPoints[i] - y) <= radiusRadius )
			{
				activeLine = lineIterator;
				allLines[lineIterator].activePoint = i;
				return true;
			}
		}
	}
	return false;
	// This uses a picking layer to find the right ones
	// Uint8 r,g,b,a;
	// int bpp = picking->format->BytesPerPixel;
    // /* Here pixel is the address to the pixel */
    // int pixel = (int)picking->pixels + y * picking->pitch + x * bpp;

	// SDL_BlitSurface( picking, NULL, surface, NULL );
	// SDL_Flip( surface );

	// SDL_GetRGBA( pixel, picking->format, &r,&g,&b,&a );
	// if( r != 255 || g != 255 || b != 255 || a != 255 )
	// {
		// activeLine = b;
		// return true;
	// }
	// return false;
}
void Bezier::move(int x, int y)
{
	if( active )
	{
		*allLines[activeLine].xPoints[allLines[activeLine].activePoint] = x;
		*allLines[activeLine].yPoints[allLines[activeLine].activePoint] = y;
	}
}
double Bezier::dist(int x, int y, int lineIndex, int pointIndex)
{
	return sqrt( (x-*allLines[lineIndex].xPoints[pointIndex])*(x-*allLines[lineIndex].xPoints[pointIndex]) + (y-*allLines[lineIndex].yPoints[pointIndex])*(y-*allLines[lineIndex].yPoints[pointIndex]) );
}
void Bezier::drawLines(void)
{
	int xNew, yNew, xOld, yOld;
	int curvePoints = 20;
	double step = 1.0/curvePoints;

	SDL_LockSurface( surface );
	SDL_FillRect( surface, NULL, 0 );	// clear surface to black
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		/* Draw control points on picking surface */
		for( int i = 0; i < 4; i++ )
		{
			filledCircleRGBA( picking, *allLines[lineIterator].xPoints[i], *allLines[lineIterator].yPoints[i], 7, 255,255,lineIterator,255 );
			/* Draw control point circles */
			circleRGBA( surface, *allLines[lineIterator].xPoints[i], *allLines[lineIterator].yPoints[i], 5, 255,255,lineIterator,255 );
		}
		
		lineRGBA( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], 128,128,128,100 );
		lineRGBA( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], 128,128,128,100 );
		lineRGBA( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[3], *allLines[lineIterator].yPoints[3], 128,128,128,100 );
		lineRGBA( surface, *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], 128,128,128,100 );
		lineRGBA( surface, *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], *allLines[lineIterator].xPoints[3], *allLines[lineIterator].yPoints[3], 128,128,128,100 );
		lineRGBA( surface, *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], *allLines[lineIterator].xPoints[3], *allLines[lineIterator].yPoints[3], 128,128,128,100 );

		/* Draw Bezier curve for current line */
		xOld = *allLines[lineIterator].xPoints[0];
		yOld = *allLines[lineIterator].yPoints[0];
		for( double t = 0+step, points = 0; points < curvePoints; t += step, points++ )
		{
			xNew = t*t*t*( *allLines[lineIterator].xPoints[3] - *allLines[lineIterator].xPoints[0] + 3*( *allLines[lineIterator].xPoints[1] - *allLines[lineIterator].xPoints[2] ) ) + 3*t*t*( *allLines[lineIterator].xPoints[0] - 2**allLines[lineIterator].xPoints[1] + *allLines[lineIterator].xPoints[2] ) - 3*t*( *allLines[lineIterator].xPoints[0] - *allLines[lineIterator].xPoints[1] ) + ( *allLines[lineIterator].xPoints[0] );
			yNew = t*t*t*( *allLines[lineIterator].yPoints[3] - *allLines[lineIterator].yPoints[0] + 3*( *allLines[lineIterator].yPoints[1] - *allLines[lineIterator].yPoints[2] ) ) + 3*t*t*( *allLines[lineIterator].yPoints[0] - 2**allLines[lineIterator].yPoints[1] + *allLines[lineIterator].yPoints[2] ) - 3*t*( *allLines[lineIterator].yPoints[0] - *allLines[lineIterator].yPoints[1] ) + ( *allLines[lineIterator].yPoints[0] );

			// circleRGBA( surface, xNew, yNew, 5, r,g,b,a/3 );
			lineRGBA( surface, xNew, yNew, xOld, yOld, r,g,b,a );

			xOld = xNew;
			yOld = yNew;
		}

	}
	SDL_UnlockSurface( surface );
	SDL_Flip( surface );
}
