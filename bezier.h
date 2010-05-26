#include <algorithm>
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

		struct pointsLines
		{
			int aLine, aPoint, dist;
		};
		bool sortPL(pointsLines,pointsLines);
	public:
		Bezier(SDL_Surface*);
		void addLine(void);		// randomly generate a line
		void addLine(int);		// make new line with user input
		void addLine(int,int,int,int,int,int,int,int);
		bool select(int,int,bool);	// accepts x,y : if close to a point, set that point active
		bool select(int,int,int,int);	// accepts x,y and oldLine,oldPoint to skip an already found point
		void move(int,int);
		double distLine(int,int,int,int);
		void drawLines(Uint32=0xFFFFFFFF);	// blank surface, then draw all lines in the structure - default color is white
		void drawLine(bLine);	// blank surface, then draw only given line
		bool active;			// true if moving a point
		bool connect(int,int);	// connects the point at (x,y) to an existing point (if near enough)
		bool disconnect(int,int);	// disconnect points near (x,y) (if near enough)
		bool splitLine(void);	// accepts input (endpoint,endpoint) and spot on curve to split it into two
		void splitLine(bLine,int,int);

		double dist(int,int,int,int);	// return distance between (x,y) and (x1,y1)
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
		*tmpBezier.xPoints[i] = event.button.x;		// initialize all points to current mouse location
		*tmpBezier.yPoints[i] = event.button.y;
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
						if( select(xMouse,yMouse,true) )		// clicked near another node : snap to that node	(WARNING! This function will change activeLine and activePoint if it returns true)
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
bool Bezier::select(int x, int y, bool adding)
{
	int radiusRadius = 64, d;	// square of radius
	pointsLines closestLine = { -1, -1, radiusRadius };
	//int xx = x * x, yy = y * y;		// squares of components
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		if( adding && lineIterator == activeLine )	// don't check current line (for snapping a new point)
			continue;
		for( int i = 0; i < 4; i++ )
		{
			d = (*allLines[lineIterator].xPoints[i] - x) * (*allLines[lineIterator].xPoints[i] - x) + (*allLines[lineIterator].yPoints[i] - y) * (*allLines[lineIterator].yPoints[i] - y);
			if( d <= closestLine.dist )
			{
                closestLine.aLine = lineIterator;
				closestLine.aPoint = i;
				closestLine.dist = d;
			}
		}
	}
	if( closestLine.aLine == -1 )
		return false;
	else		// find nearest point from (x,y)
	{
		activeLine = closestLine.aLine;
		allLines[activeLine].activePoint = closestLine.aPoint;
		return true;
	}
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
/* This select function will select a point provided it is not the given point */
bool Bezier::select(int x, int y, int givenLine, int givenPoint)
{
	int radiusRadius = 64, d;	// square of radius
	pointsLines closestLine = { -1, -1, radiusRadius };
	//int xx = x * x, yy = y * y;		// squares of components
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		for( int i = 0; i < 4; i++ )
		{
			if( lineIterator == givenLine && i == givenPoint )
				continue;
			d = (*allLines[lineIterator].xPoints[i] - x) * (*allLines[lineIterator].xPoints[i] - x) + (*allLines[lineIterator].yPoints[i] - y) * (*allLines[lineIterator].yPoints[i] - y);
			if( d <= closestLine.dist )
			{
                closestLine.aLine = lineIterator;
				closestLine.aPoint = i;
				closestLine.dist = d;
			}
		}
	}
	if( closestLine.aLine == -1 )
		return false;
	else		// find nearest point from (x,y)
	{
		activeLine = closestLine.aLine;
		allLines[activeLine].activePoint = closestLine.aPoint;
		return true;
	}
}
inline bool Bezier::sortPL(pointsLines left, pointsLines right)
{
	return left.dist <= right.dist;		// left goes before right (return true) otherwise (return false)
}
void Bezier::move(int x, int y)
{
	if( active )
	{
		*allLines[activeLine].xPoints[allLines[activeLine].activePoint] = x;
		*allLines[activeLine].yPoints[allLines[activeLine].activePoint] = y;
	}
}
double Bezier::distLine(int x, int y, int lineIndex, int pointIndex)
{
	return sqrt( (x-*allLines[lineIndex].xPoints[pointIndex])*(x-*allLines[lineIndex].xPoints[pointIndex]) + (y-*allLines[lineIndex].yPoints[pointIndex])*(y-*allLines[lineIndex].yPoints[pointIndex]) );
}
void Bezier::drawLines(Uint32 color)
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
			// filledCircleRGBA( picking, *allLines[lineIterator].xPoints[i], *allLines[lineIterator].yPoints[i], 7, 255,255,lineIterator,255 );
			/* Draw control point circles */
			// circleRGBA( surface, *allLines[lineIterator].xPoints[i], *allLines[lineIterator].yPoints[i], 5, 255,255,255,255 );
			circleColor( surface, *allLines[lineIterator].xPoints[i], *allLines[lineIterator].yPoints[i], 5, color - 0xF );
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
			// lineRGBA( surface, xNew, yNew, xOld, yOld, r,g,b,a );
			lineColor( surface, xNew, yNew, xOld, yOld, color );

			xOld = xNew;
			yOld = yNew;
		}

	}
	SDL_UnlockSurface( surface );
	SDL_Flip( surface );
}
bool Bezier::disconnect(int x, int y)
{
	bLine tmpLine = allLines[activeLine];
	int oldPoint, oldLine;
	if( select( x, y, false ) )		// found one point under (x,y)
	{
		oldPoint = allLines[activeLine].activePoint;
		oldLine  = activeLine;
		if( select( x, y, oldLine, oldPoint ) )		// found second point under (x,y)
		{
			allLines[oldLine].xPoints[oldPoint] = new int;
			*allLines[oldLine].xPoints[oldPoint] = *allLines[activeLine].xPoints[allLines[activeLine].activePoint];		// copy second point location to first point
			allLines[oldLine].yPoints[oldPoint] = new int;
			*allLines[oldLine].yPoints[oldPoint] = *allLines[activeLine].yPoints[allLines[activeLine].activePoint];

			activeLine = oldLine;		// reset closest point to be active again
			allLines[activeLine].activePoint = oldPoint;
			return true;
		}
		else
			return false;		// only one point found
	}
	else
		return false;	// no points found near (x,y)
}
/* Caution: use this only when a point is active, or it will do no good */
bool Bezier::connect(int x, int y)
{
	if( !active )	// not moving a point
		return false;
	int oldPoint = allLines[activeLine].activePoint;	// get current point and line index
	int oldLine  = activeLine;
	if( select( x, y, oldLine, oldPoint ) )		// found a point near the current one (excluding active point)
	{
		allLines[oldLine].xPoints[oldPoint] = allLines[activeLine].xPoints[allLines[activeLine].activePoint];
		allLines[oldLine].yPoints[oldPoint] = allLines[activeLine].yPoints[allLines[activeLine].activePoint];
		return true;
	}
	else
		return false;
}
void Bezier::drawLine(bLine bl)
{
	int xNew, yNew, xOld, yOld;
	int curvePoints = 20;
	double step = 1.0/curvePoints;

	SDL_LockSurface( surface );
	// SDL_FillRect( surface, NULL, 0 );	// clear surface to black
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		/* Draw control points on picking surface */
		for( int i = 0; i < 4; i++ )
		{
			/* Draw control point circles */
			// circleRGBA( surface, *bl.xPoints[i], *bl.yPoints[i], 5, 255,255,lineIterator,255 );
			circleColor( surface, *bl.xPoints[i], *bl.yPoints[i], 5, 0xFFFFFF3F );
		}

		lineRGBA( surface, *bl.xPoints[0], *bl.yPoints[0], *bl.xPoints[1], *bl.yPoints[1], 128,128,128,100 );
		lineRGBA( surface, *bl.xPoints[0], *bl.yPoints[0], *bl.xPoints[2], *bl.yPoints[2], 128,128,128,100 );
		lineRGBA( surface, *bl.xPoints[0], *bl.yPoints[0], *bl.xPoints[3], *bl.yPoints[3], 128,128,128,100 );
		lineRGBA( surface, *bl.xPoints[1], *bl.yPoints[1], *bl.xPoints[2], *bl.yPoints[2], 128,128,128,100 );
		lineRGBA( surface, *bl.xPoints[1], *bl.yPoints[1], *bl.xPoints[3], *bl.yPoints[3], 128,128,128,100 );
		lineRGBA( surface, *bl.xPoints[2], *bl.yPoints[2], *bl.xPoints[3], *bl.yPoints[3], 128,128,128,100 );

		/* Draw Bezier curve for current line */
		xOld = *bl.xPoints[0];
		yOld = *bl.yPoints[0];
		for( double t = 0+step, points = 0; points < curvePoints; t += step, points++ )
		{
			xNew = t*t*t*( *bl.xPoints[3] - *bl.xPoints[0] + 3*( *bl.xPoints[1] - *bl.xPoints[2] ) ) + 3*t*t*( *bl.xPoints[0] - 2**bl.xPoints[1] + *bl.xPoints[2] ) - 3*t*( *bl.xPoints[0] - *bl.xPoints[1] ) + ( *bl.xPoints[0] );
			yNew = t*t*t*( *bl.yPoints[3] - *bl.yPoints[0] + 3*( *bl.yPoints[1] - *bl.yPoints[2] ) ) + 3*t*t*( *bl.yPoints[0] - 2**bl.yPoints[1] + *bl.yPoints[2] ) - 3*t*( *bl.yPoints[0] - *bl.yPoints[1] ) + ( *bl.yPoints[0] );

			// circleRGBA( surface, xNew, yNew, 5, r,g,b,a/3 );
			// lineColor( surface, xNew, yNew, xOld, yOld, 0xFFFFFFFF );
			lineRGBA( surface, xNew, yNew, xOld, yOld, 128,128,128,255 );
			// circleRGBA( surface, xNew, yNew, 3, 255,255,255,255 );

			xOld = xNew;
			yOld = yNew;
		}

	}
	SDL_UnlockSurface( surface );
	SDL_Flip( surface );
}
bool Bezier::splitLine(void)
{
	int selectedPoints = 0, firstLine, firstPoint, secondLine, secondPoint;
	int oldPoint, oldLine, xMouse, yMouse;
	bool run = true, selected = false;
	bLine bl;
	SDL_Event event;
	while( run )
	{
		while( SDL_PollEvent(&event) )		// get events
		{
			switch( event.type )
			{
				case SDL_KEYUP:				// keyboard released
					if( event.key.keysym.sym == SDLK_ESCAPE )
						return false;	// break out of splitting - this should reinstate the chosen line (to be done later)
				case SDL_MOUSEBUTTONDOWN:	// mouse pressed
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						xMouse = event.button.x;	// get mouse click location
						yMouse = event.button.y;	//

						// the select function needs some help - if two lines are connected to a single node, it will just select an arbitrary line (which one should it choose?) - maybe click on node, then click on line from node
						if( select(xMouse,yMouse,false) )		// clicked near another node : snap to that node	(WARNING! This function will change activeLine and activePoint if it returns true)
						{
							selected = true;
							bl = allLines[activeLine];	// copy chosen line to a temporary one
							drawLines( SDL_MapRGBA(surface->format, 100,100,100,100) );
							drawLine( bl );
							allLines.erase(allLines.begin() + activeLine);	// remove line to be split (it is now in bl)
							run = false;
							
							std::cout << "Check\n";
						}
						break;
					}
					else if( event.button.button == SDL_BUTTON_RIGHT )
						return false;
				// case SDL_MOUSEMOTION:
					// if( select( event.motion.x, event.motion.y, false ) )
						// fill in here with code to highlight a near node
					// break;
				case SDL_QUIT:		// top-right X clicked
					return false;
				default:
					break;
			}
		}
	}
	/* Click code - click near a line to select a spot on the line */
	// now the active line is in bl
	while( true )
	{
		while( SDL_PollEvent(&event) )
		{
			switch( event.type )
			{
				case SDL_MOUSEBUTTONDOWN:
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						for( int i = -2; i <= 2; i++ )
						{
							for( int j = -2; j <= 2; j++ )
							{
								if( (i-j)*(i-j) > 2*2 )	// exclude edges of i X j square not in a circle (huh?)
									continue;
								if( getpixel( surface, event.button.x + i, event.button.y + j ) == SDL_MapRGBA(surface->format, 255,255,255,255) )
								{
									std::cout << "Found white pixel\n";
									splitLine( bl, event.button.x + i, event.button.y + j );
									drawLines();
									return true;
								}
					}	}	}		// what about this notation for ending a set of brackets? heh heh...
					break;
				case SDL_MOUSEBUTTONUP:
					if( event.button.button == SDL_BUTTON_RIGHT )
						return false;
					break;
				case SDL_KEYUP:
					if( event.key.keysym.sym == SDLK_ESCAPE )
						return false;
					break;
				case SDL_QUIT:
					return false;
			}
		}
	}
}
/* This function splits a curve at approximately (x,y) */
void Bezier::splitLine( bLine bl, int x, int y )
{
	int xNew, yNew;
	int curvePoints = 20;
	double step = 1.0/curvePoints;
	bLine first, second;
	for( int i = 0; i < 4; i++ )
	{
		first.xPoints[i] = new int;
		first.yPoints[i] = new int;
		second.xPoints[i] = new int;
		second.yPoints[i] = new int;
	}
	first.activePoint = second.activePoint = 0;
	// this now needs to be able to home in on a specific point
	for( double t = 0; t <= 1; t += step )
	{
		xNew = t*t*t*( *bl.xPoints[3] - *bl.xPoints[0] + 3*( *bl.xPoints[1] - *bl.xPoints[2] ) ) + 3*t*t*( *bl.xPoints[0] - 2**bl.xPoints[1] + *bl.xPoints[2] ) - 3*t*( *bl.xPoints[0] - *bl.xPoints[1] ) + ( *bl.xPoints[0] );
		yNew = t*t*t*( *bl.yPoints[3] - *bl.yPoints[0] + 3*( *bl.yPoints[1] - *bl.yPoints[2] ) ) + 3*t*t*( *bl.yPoints[0] - 2**bl.yPoints[1] + *bl.yPoints[2] ) - 3*t*( *bl.yPoints[0] - *bl.yPoints[1] ) + ( *bl.yPoints[0] );
		
		if( dist(x,y,xNew,yNew) <= 2*2 )	// distance from given (x,y) to curve (x,y) is within 2 pixels
		{
			std::cout << "Splitting curve\n";
			// now split the curve
			 *first.xPoints[0] = *bl.xPoints[0];
			 *first.xPoints[1] = (1-t)* *bl.xPoints[0]+t* *bl.xPoints[1];
			 *first.xPoints[2] = (1-t)*((1-t)* *bl.xPoints[0]+t* *bl.xPoints[1])+t*((1-t)* *bl.xPoints[1]+t* *bl.xPoints[2]);
			 *first.xPoints[3] = (1-t)*((1-t)*((1-t)* *bl.xPoints[0]+t* *bl.xPoints[1])+t*((1-t)* *bl.xPoints[1]+t* *bl.xPoints[2]))+t*((1-t)*((1-t)* *bl.xPoints[1]+t* *bl.xPoints[2])+t*((1-t)* *bl.xPoints[2]+t* *bl.xPoints[3]));
			 *first.yPoints[0] = *bl.yPoints[0];
			 *first.yPoints[1] = (1-t)* *bl.yPoints[0]+t* *bl.yPoints[1];
			 *first.yPoints[2] = (1-t)*((1-t)* *bl.yPoints[0]+t* *bl.yPoints[1])+t*((1-t)* *bl.yPoints[1]+t* *bl.yPoints[2]);
			 *first.yPoints[3] = (1-t)*((1-t)*((1-t)* *bl.yPoints[0]+t* *bl.yPoints[1])+t*((1-t)* *bl.yPoints[1]+t* *bl.yPoints[2]))+t*((1-t)*((1-t)* *bl.yPoints[1]+t* *bl.yPoints[2])+t*((1-t)* *bl.yPoints[2]+t* *bl.yPoints[3]));
			 
			 second.xPoints[0] = first.xPoints[3];	// connect midpoint
			 second.yPoints[0] = first.yPoints[3];	// 
			 
			// *second.xPoints[0] = (1-t)*((1-t)*((1-t)* *bl.xPoints[0]+t* *bl.xPoints[1])+t*((1-t)* *bl.xPoints[1]+t* *bl.xPoints[2]))+t*((1-t)*((1-t)* *bl.xPoints[1]+t* *bl.xPoints[2])+t*((1-t)* *bl.xPoints[2]+t* *bl.xPoints[3]));
			*second.xPoints[1] = (1-t)*((1-t)* *bl.xPoints[1]+t* *bl.xPoints[2])+t*((1-t)* *bl.xPoints[2]+t* *bl.xPoints[3]);
			*second.xPoints[2] = (1-t)* *bl.xPoints[2]+t* *bl.xPoints[3];
			*second.xPoints[3] = *bl.xPoints[3];
			// *second.yPoints[0] = (1-t)*((1-t)*((1-t)* *bl.yPoints[0]+t* *bl.yPoints[1])+t*((1-t)* *bl.yPoints[1]+t* *bl.yPoints[2]))+t*((1-t)*((1-t)* *bl.yPoints[1]+t* *bl.yPoints[2])+t*((1-t)* *bl.yPoints[2]+t* *bl.yPoints[3]));
			*second.yPoints[1] = (1-t)*((1-t)* *bl.yPoints[1]+t* *bl.yPoints[2])+t*((1-t)* *bl.yPoints[2]+t* *bl.yPoints[3]);
			*second.yPoints[2] = (1-t)* *bl.yPoints[2]+t* *bl.yPoints[3];
			*second.yPoints[3] = *bl.yPoints[3];
			allLines.push_back( first );
			allLines.push_back( second );
			break;
		}
	}
	std::cout << "Ended search\n";
}
inline double Bezier::dist(int x0, int y0, int x1, int y1)
{
	return (sqrt((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0)));
}
/*
Using de Casteljau's algorithm
Source : http://www.genie-meca.ac-aix-marseille.fr/Productique/PDF/361_deCasteljau_john.pdf

I worked this out with much labor and pain

p_0^0(t) = x0,y0
p_0^1(t) = (1-t)*p_0+t*p_1
p_0^2(t) = (1-t)*((1-t)*p_0+t*p_1)+t*((1-t)*p_1+t*p_2)

p_0^3(t) = (1-t)*((1-t)*((1-t)*p_0+t*p_1)+t((1-t)*p_1+t*p_2))+t((1-t)((1-t)*p_1+t*p_2)+t((1-t)*p_2+t*p_3))		which is an endpoint for each curve

p_1^2(t) = (1-t)*((1-t)*p_1+t*p_2)+t*((1-t)*p_2+t*p_3)
p_2^1(t) = (1-t)*p_2+t*p_3
p_3^0(t) = x3,y3

p_1^0(t) = x1,y1
p_2^0(t) = x2,y2

New points
[p_0,p_0^1,p_0^2,p_0^3] and [p_0^3,p_1^2,p_2^1,p_3^0]

Steps for getting the two new curves:
	Iterate through values of t until close to the desired (x,y)
	Create the two new curves for that value of t

No more recursion for quadratic Bezier curves!

Mike
 34-35 sleeve
 15.5 collar
 Medium

*/
