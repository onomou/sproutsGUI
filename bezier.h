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
		void addLine(void);
		void addLine(int,int,int,int,int,int,int,int);
		bool select(int,int);
		void getClosest(int,int,int&,int&);
		double dist(int,int,int,int);
		void drawLines(void);
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
		std::cout << *tmpBezier.yPoints[i] << std::endl;
	}
	tmpBezier.activePoint = 0;

	allLines.push_back(tmpBezier);
	activeLine = allLines.size() - 1;

	/* Set colors */
	r = g = b = a = 255;
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
		std::cout << *tmpBezier.yPoints[i] << std::endl;
	}
	tmpBezier.activePoint = 0;

	allLines.push_back(tmpBezier);
	activeLine = allLines.size() - 1;
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
	Uint8 r,g,b,a;
	int bpp = picking->format->BytesPerPixel;
    /* Here pixel is the address to the pixel */
    int pixel = (int)picking->pixels + y * picking->pitch + x * bpp;

	SDL_BlitSurface( picking, NULL, surface, NULL );
	SDL_Flip( surface );

	SDL_GetRGBA( pixel, picking->format, &r,&g,&b,&a );
	if( r != 255 || g != 255 || b != 255 || a != 255 )
	{
		activeLine = b;
		return true;
	}
	return false;
}
void Bezier::getClosest(int mouseX, int mouseY, int &lineIndex, int &pointIndex)
{
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
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		/* Draw control points on picking surface */
		for( int i = 0; i < 4; i++ )
		{
			filledCircleRGBA( picking, *allLines[lineIterator].xPoints[i], *allLines[lineIterator].yPoints[i], 7, 255,255,lineIterator,255 );
		}

		/* Draw Bezier curve for current line */
		xOld = *allLines[lineIterator].xPoints[0];
		yOld = *allLines[lineIterator].yPoints[0];
		for( double t = 0+step; t <= 1; t += step )
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
