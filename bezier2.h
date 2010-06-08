#include <algorithm>
#include <ctime>
#include <vector>
#include <cmath>
#include "xorRNG.h"
#include "draw.h"

/* This is a quadratic Bezier */
class Bezier
{
	private:

		struct point
		{
			int xPoint,yPoint,degree;
		};
		std::vector<point> spots;
		int activeSpot;

		/* Global constants for this class  */
		static const int r = 255,
						 g = 255,
						 b = 255,
						 a = 255;
		static const int curvePoints = 40;
		static const int circleRadius = 5;
		static const int selectRadius = 15;
		static const int selectRadius2 = 225;
		static const Uint32 defaultColor = 0xFFFFFFFF;		//
		static const Uint32 grayedColor = 0xD4D4D480;		// assuming big-endian?
		static const Uint32 highlightColor = 0xFF00FFFF;	//
		static const int numSpots = 4;
		/* End constants */

		struct pointsLines		// for associating an active line, point pair and its distance from some other point
		{ int aLine, aPoint, dist; };

		struct bLine
		{
			int *xPoints[3];	// array of 3 pointers to integers
			int *yPoints[3];	//
			int activePoint;
		};
		std::vector<bLine> allLines;
		int activeLine;

		SDL_Surface *surface;	// screen to draw onto
		SDL_Surface *picking;	// may be used for selecting nodes at some point (if checking all distances becomes too slow) : NOT USED CURRENTLY

		bool doLockSurface;		// for internal use - make this false (be sure to reset it) if multiple draw commands need to be executed before flipping the surface
		bool highlighted;		// true iff mouse is near a point

		/* Private functions */
		double dist(int,int,int,int);					// return distance between (x0,y0) and (x1,y1)
		int dist2(int,int,int,int);						// return square of distance between (x0,y0) and (x1,y1)
		void drawCircles(bLine&,Uint32=defaultColor);	// draw curve's circles, default color white

	public:
		/* Public variables */
		bool active;					// true if moving a point

		/* Constructors */
		Bezier(SDL_Surface*);			// default constructor

		/* Curve generation */
		void addLine(void);				// make new line with user input - only snap to existing nodes

		/* Curve selection */
		void highlightNear(int,int);	// highlights a node if input near enough to select it - call drawLines() first
		bool select(int,int);			// accepts x,y : if close to a point, set that point active - bool value controls whether to select the active point
		bool select(int,int,int);		// accepts x,y and a point index to skip an already found point

		/* Curve operations */
		void move(int,int);				// move active point to (x,y)
		bool splitLine(void);			// accepts input (endpoint,endpoint) and spot on curve to split it into two
		void splitLine(bLine,int,int);

		/* Curve visualization */
		void drawLines(Uint32=defaultColor);	// blank surface, then draw all lines in the structure - default color is white
		void drawLine(bLine);					// blank surface, then draw only given line
		void drawSpots(Uint32=defaultColor);
};
Bezier::Bezier(SDL_Surface *sf)
{
	srand( time(NULL) );
	point *tmpSpot;

	/* Set drawing surface */
	surface = sf;
	picking = SDL_ConvertSurface(sf, sf->format, sf->flags);	// picking layer to select a line
	SDL_FillRect( picking, NULL, 0 );	// blank picking surface

	/* Scatter spots randomly */
	tmpSpot = new point;
	tmpSpot->degree = 0;
	for( int i = 0; i < numSpots; i++ )
	{
		tmpSpot->xPoint = random( (surface->w * 3)/4 ) + surface->w / 8;
		tmpSpot->yPoint = random( (surface->h * 3)/4 ) + surface->h / 8;
		spots.push_back( *tmpSpot );

		filledCircleColor( picking, (tmpSpot->xPoint), (tmpSpot->yPoint), selectRadius, defaultColor );
	}

	drawSpots();
	active = false;		// not moving a point initially
	doLockSurface = true;
	highlighted = false;
}
void Bezier::addLine()
{
	bool run = true, drawn = true;
	int pointsDone = 0;
	bLine tmpBezier;
	tmpBezier.activePoint = 0;
	SDL_Event event;
	SDL_PollEvent(&event);
	while( pointsDone < 3 && run )
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
						if( pointsDone == 0 )	// no points yet selected
						{
							if( select( event.button.x, event.button.y ) )	// clicked near a point (point is located by activeSpot)
							{
								*tmpBezier.xPoints[0] = spots[activeSpot].xPoint;
								*tmpBezier.yPoints[0] = spots[activeSpot].yPoint;
								pointsDone++;
							}
						}
						else if( pointsDone == 1 )
						{
							if( select( event.button.x, event.button.y ) )	// clicked near the second endpoint
							{
								*tmpBezier.xPoints[2] = spots[activeSpot].xPoint;
								*tmpBezier.yPoints[2] = spots[activeSpot].yPoint;
								pointsDone++;
								circleColor( surface, *tmpBezier.xPoints[2], *tmpBezier.yPoints[2], circleRadius, defaultColor );
							}
						}
						else if( pointsDone == 2 )
						{
							tmpBezier.xPoints[1] = event.button.x;
							tmpBezier.yPoints[1] = event.button.y;
							// allLines.push_back( tmpBezier );
							run = false;
						}
					}
					else if( event.button.button == SDL_BUTTON_RIGHT )
					{
						// select( event.button.x, event.button.y );
						run = false;
					}
					break;
				case SDL_MOUSEMOTION:		// mouse moved

					// add a section to display the curve if two points have already been selected

					doLockSurface = false;
					SDL_LockSurface( surface );
					if( pointsDone < 2 )	// only highlight spots if snapping to one (first and second selections)
					{
						highlightNear(event.motion.x, event.motion.y);
						if( !drawn )
						{
							circleColor( surface, event.motion.x, event.motion.y, circleRadius, defaultColor );
							drawn = true;
						}
					}
					doLockSurface = true;
					SDL_UnlockSurface( surface );
					SDL_Flip( surface );
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
	}
	// drawLines();
}
void Bezier::highlightNear(int x, int y)
{
	if( select(x,y) )	// found a near spot, not indexed by activeSpot
	{
		if( !highlighted )	// not previously highlighted, now mouse near a spot
		{
			circleColor( surface, spots[activeSpot].xPoint, spots[activeSpot].yPoint, circleRadius, highlightColor );
			highlighted = true;
		}
	}
	else if( highlighted )	// was highlighted, now mouse not near that node
	{
		circleColor( surface, spots[activeSpot].xPoint, spots[activeSpot].yPoint, circleRadius, defaultColor );
		highlighted = false;
	}
	if( doLockSurface )
		SDL_Flip( surface );
}
// http://www.libsdl.org/intro.en/usingvideo.html
bool Bezier::select(int x, int y)
{
	if( spots.empty() )
		return false;
	int closestPoint = 0;
	int closestRadius = surface->w, tmpRadius;
	for( int spotIt = 0; spotIt < spots.size(); spotIt++ )
	{
		tmpRadius = dist2( x, y, spots[spotIt].xPoint, spots[spotIt].yPoint );
		if( tmpRadius < closestRadius )
		{
			closestRadius = tmpRadius;
			closestPoint  = spotIt;
		}
	}
	if( closestRadius <= selectRadius2 )
	{
		activeSpot = closestPoint;
		return true;
	}
	else
		return false;
}
/* This select function will select a point provided it is not the given point */
// don't need this function?
bool Bezier::select(int x, int y, int givenPoint)
{
	if( spots.empty() )
		return false;
	int closestPoint = 0;
	int closestRadius = dist2( x, y, spots[0].xPoint, spots[0].yPoint ), tmpRadius;
	for( int spotIt = 0; spotIt < spots.size(); spotIt++ )
	{
		if( spotIt == givenPoint )
			continue;
		tmpRadius = dist2( x, y, spots[spotIt].xPoint, spots[spotIt].yPoint );
		if( tmpRadius <= closestRadius )
		{
			closestRadius = tmpRadius;
			closestPoint  = spotIt;
		}
	}
	if( closestRadius <= selectRadius2 )
	{
		activeSpot = closestPoint;
		return true;
	}
	else
		return false;
}
// don't need this function
void Bezier::move(int x, int y)
{
	if( active )
	{
		*allLines[activeLine].xPoints[allLines[activeLine].activePoint] = x;
		*allLines[activeLine].yPoints[allLines[activeLine].activePoint] = y;
	}
}
/* This function splits a curve at approximately (x,y) */
void Bezier::splitLine( bLine bl, int x, int y )
{
	int xNew, yNew;
	double smallestDistance = dist( x,y, *bl.xPoints[0],*bl.yPoints[0] ), smallestT;
	double step = 1.0/curvePoints;
	bLine first, second;
	point tmpSpot;

	for( int i = 0; i < 3; i++ )
	{
		first.xPoints[i] = new int;
		first.yPoints[i] = new int;
		second.xPoints[i] = new int;
		second.yPoints[i] = new int;
	}
	first.activePoint = second.activePoint = 0;
	// find t value at which distance to target is minimum
	for( double t = 0; t <= 1; t += step )
	{
		xNew = (1-t)*(1-t)* *bl.xPoints[0] + 2*(1-t)*t* *bl.xPoints[1] + t*t* *bl.xPoints[2];
		yNew = (1-t)*(1-t)* *bl.yPoints[0] + 2*(1-t)*t* *bl.yPoints[1] + t*t* *bl.yPoints[2];

		if( dist(x,y,xNew,yNew) < smallestDistance )	// distance from given (x,y) to curve (x,y) is within 2 pixels
		{
			smallestDistance = dist( x,y, xNew,yNew );
			smallestT = t;
		}
	}
	// now split the curve

	tmpSpot.xPoint = (1-smallestT)*((1-smallestT)*(*bl.xPoints[0]) + smallestT*(*bl.xPoints[1])) + smallestT*((1-smallestT)*(*bl.xPoints[1]) + smallestT*(*bl.xPoints[2]));
	tmpSpot.yPoint = (1-smallestT)*((1-smallestT)*(*bl.yPoints[0]) + smallestT*(*bl.yPoints[1])) + smallestT*((1-smallestT)*(*bl.yPoints[1]) + smallestT*(*bl.yPoints[2]));
	tmpSpot.degree = 2;
	spots.push_back( tmpSpot );

	 *first.xPoints[0] = *bl.xPoints[0];
	 *first.xPoints[1] = (1-smallestT)*(*bl.xPoints[0]) + smallestT*(*bl.xPoints[1]);
	  first.xPoints[2] = &(spots.back().xPoint);
	 *first.yPoints[0] = *bl.yPoints[0];
	 *first.yPoints[1] = (1-smallestT)*(*bl.yPoints[0]) + smallestT*(*bl.yPoints[1]);
	  first.yPoints[2] = &(spots.back().yPoint);

	 second.xPoints[0] = &(spots.back().xPoint);
	*second.xPoints[1] = (1-smallestT)*(*bl.xPoints[1]) + smallestT*(*bl.xPoints[2]);
	*second.xPoints[2] = *bl.xPoints[2];
	 second.yPoints[0] = &(spots.back().yPoint);
	*second.yPoints[1] = (1-smallestT)*(*bl.yPoints[1]) + smallestT*(*bl.yPoints[2]);
	*second.yPoints[2] = *bl.yPoints[2];

	allLines.push_back( first );
	allLines.push_back( second );
}
void Bezier::drawLines(Uint32 color)
{
	int xNew, yNew, xOld, yOld;
	double step = 1.0/curvePoints;

    if( doLockSurface )
        SDL_LockSurface( surface );

	SDL_FillRect( surface, NULL, 0 );	// clear surface to black
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		/* Draw control points on picking surface */
		drawCircles( allLines[lineIterator], color & grayedColor );


		lineColor( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], grayedColor );
		lineColor( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], grayedColor );
		lineColor( surface, *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], grayedColor );

		/* Draw Bezier curve for current line */
		xOld = *allLines[lineIterator].xPoints[0];
		yOld = *allLines[lineIterator].yPoints[0];
		for( double t = 0+step, points = 0; points < curvePoints; t += step, points++ )
		{
			xNew = (1-t)*(1-t)* *allLines[lineIterator].xPoints[0] + 2*(1-t)*t* *allLines[lineIterator].xPoints[1] + t*t* *allLines[lineIterator].xPoints[2];
			yNew = (1-t)*(1-t)* *allLines[lineIterator].yPoints[0] + 2*(1-t)*t* *allLines[lineIterator].yPoints[1] + t*t* *allLines[lineIterator].yPoints[2];

			for( int i = -2; i <= 2; i++ )
				for( int j = -2; j <= 2; j++ )
					lineColor( picking, xNew+i, yNew+j, xOld+i, yOld+j, color );

			lineColor( surface, xNew, yNew, xOld, yOld, color );

			xOld = xNew;
			yOld = yNew;
		}
		if( active )	// dragging a point - highlight that point
			circleColor( surface, *allLines[activeLine].xPoints[allLines[activeLine].activePoint], *allLines[activeLine].yPoints[allLines[activeLine].activePoint], circleRadius, highlightColor );
	}
    drawSpots();
	if( doLockSurface )
    {
        SDL_UnlockSurface( surface );
        SDL_Flip( surface );
    }
}
/* TODO: Use this function to draw new lines in conjunction with error checking */
void Bezier::drawLine(bLine bl)
{
	int xNew, yNew, xOld, yOld;
	double step = 1.0/curvePoints;

	if( doLockSurface )
		SDL_LockSurface( surface );
	// SDL_FillRect( surface, NULL, 0 );	// clear surface to black
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		/* Draw control points on surface */
		for( int i = 0; i < 3; i++ )
		{
			/* Draw control point circles */
			circleColor( surface, *bl.xPoints[i], *bl.yPoints[i], circleRadius, 0xFFFFFF3F );
		}

		lineColor( surface, *bl.xPoints[0], *bl.yPoints[0], *bl.xPoints[1], *bl.yPoints[1], grayedColor );
		lineColor( surface, *bl.xPoints[0], *bl.yPoints[0], *bl.xPoints[2], *bl.yPoints[2], grayedColor );
		lineColor( surface, *bl.xPoints[1], *bl.yPoints[1], *bl.xPoints[2], *bl.yPoints[2], grayedColor );

		/* Draw Bezier curve for current line */
		xOld = *bl.xPoints[0];
		yOld = *bl.yPoints[0];
		for( double t = 0+step, points = 0; points < curvePoints; t += step, points++ )
		{
			xNew = (1-t)*(1-t)* *allLines[lineIterator].xPoints[0] + 2*(1-t)*t* *allLines[lineIterator].xPoints[1] + t*t* *allLines[lineIterator].xPoints[2];
			yNew = (1-t)*(1-t)* *allLines[lineIterator].yPoints[0] + 2*(1-t)*t* *allLines[lineIterator].yPoints[1] + t*t* *allLines[lineIterator].yPoints[2];


			for( int i = -2; i <= 2; i++ )
				for( int j = -2; j <= 2; j++ )
					lineColor( picking, xNew+i, yNew+j, xOld+i, yOld+j, defaultColor );

			lineColor( surface, xNew, yNew, xOld, yOld, defaultColor );

			xOld = xNew;
			yOld = yNew;
		}

	}
	if( doLockSurface )
	{
		SDL_UnlockSurface( surface );
		SDL_Flip( surface );
	}
}
void Bezier::drawSpots(Uint32 color)
{
	if( doLockSurface )
		SDL_LockSurface( surface );
	for( int i = 0; i < spots.size(); i++ )
	{
		circleColor( surface, spots[i].xPoint, spots[i].yPoint, circleRadius, color );
	}
	if( doLockSurface )
	{
		SDL_UnlockSurface( surface );
		SDL_Flip( surface );
	}
}
inline double Bezier::dist(int x0, int y0, int x1, int y1)
{
	return (sqrt((double)dist2(x0, y0, x1, y1)));
}
inline int Bezier::dist2(int x0, int y0, int x1, int y1)
{
	return ((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));
}
/* Lock the surface before calling this function */
inline void Bezier::drawCircles(bLine &bl, Uint32 color)
{
	if( doLockSurface )
		SDL_LockSurface( surface );
	for( int i = 0; i < 3; i++ )
	{
		circleColor( surface, *bl.xPoints[i], *bl.yPoints[i], circleRadius, color );
	}
	if( doLockSurface )
	{
		SDL_UnlockSurface( surface );
		SDL_Flip( surface );
	}
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

From split function above (reinstate this if the split point should not be linked to both new curves)
	*second.xPoints[0] = (1-t)*((1-t)*((1-t)* *bl.xPoints[0]+t* *bl.xPoints[1])+t*((1-t)* *bl.xPoints[1]+t* *bl.xPoints[2]))+t*((1-t)*((1-t)* *bl.xPoints[1]+t* *bl.xPoints[2])+t*((1-t)* *bl.xPoints[2]+t* *bl.xPoints[3]));
	*second.yPoints[0] = (1-t)*((1-t)*((1-t)* *bl.yPoints[0]+t* *bl.yPoints[1])+t*((1-t)* *bl.yPoints[1]+t* *bl.yPoints[2]))+t*((1-t)*((1-t)* *bl.yPoints[1]+t* *bl.yPoints[2])+t*((1-t)* *bl.yPoints[2]+t* *bl.yPoints[3]));


Mike
 34-35 sleeve
 15.5 collar
 Medium

Bill
 15-15.5
 34-35
 Medium

Tommy
 32-33-34
 14.5-15
 Medium

*/
