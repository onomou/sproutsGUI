#include <vector>
#include "xorRNG.h"
#include "draw.h"

#include <iostream>

class Sprout
{
	private:
		/* Constants */
		static const Uint32 defaultColor = 0xFFFFFFFF;		//
		static const Uint32 grayedColor = 0xD4D4D480;		// assuming big-endian?
		static const Uint32 highlightColor = 0xFF00FFFF;	//
		static const int spotRadius = 5;
		static const int selectRadius = 15;
		static const int thickness = 1;		// thickness for drawing lines and points

		/* Structures and other storage stuff */
		struct spot   { int xPoint,yPoint,degree; };
		struct connection { std::vector<int*> xPoints, yPoints; };

		std::vector<spot*>  spots;	// all the spots
		std::vector<connection*> lines;	// all the lines

		int activeSpot;
		
		/* Variables */
		SDL_Surface *surface;	// screen for drawing
		bool doLockSurface;		// for internal use - make this false (be sure to reset it) if multiple draw commands need to be executed before flipping the surface
		bool highlighted;

		/* Private functions */
		void thickLine(SDL_Surface*,int,int,int,int,Uint32);	// draws a line of thickness n+2 (last parameter n)
		  void connect(int,int,int,int);	// (by index) connect first spot to second spot through (x,y)
		  void drawSpots(void);		// draw all the spots
		  void drawConnection(connection*);		// draw the current temporary line
		  bool select(int,int);			// set spot active it near x,y
		double dist(int,int,int,int);	// return distance between (x0,y0) and (x1,y1)
		   int dist2(int,int,int,int);	// return square of distance between (x0,y0) and (x1,y1)
		  bool lineValid(int,int,int,int);	// draw line segments from x0,y0 to x,y : returns false if hit another line
	public:
		Sprout(SDL_Surface*,int);	// default constructor : feed it a surface and initial spots number

        void drawLines(void);
		bool highlightNear(int,int);	// highlight spot if near x,y (calls select())
		bool connect(void);				// loop to connect two spots
};
Sprout::Sprout(SDL_Surface *sf, int numSprouts)
{
	doLockSurface = true;
	highlighted = false;
	surface = sf;

	/* Set up initial board (seed sprouts) */
	spot *tmpSpot;
	for( int i = 0; i < numSprouts; i++ )
	{
		tmpSpot = new spot;
		tmpSpot->degree = 0;
		tmpSpot->xPoint = random( (surface->w)/2 ) + surface->w / 4;
		tmpSpot->yPoint = random( (surface->h)/2 ) + surface->h / 4;
		spots.push_back( tmpSpot );
	}
	drawSpots();
	/* End spots setup */
}
void Sprout::thickLine(SDL_Surface *sf, int x0, int y0, int x, int y, Uint32 color)
{
	for( int i = -thickness; i <= thickness; i++ )
	{
		for( int j = -thickness; j <= thickness; j++ )
		{
			lineColor( sf, x0+i, y0+j, x+i, y+j, color );
		}
	}
}
bool Sprout::connect(void)
{
	SDL_Event event;			// dump event polls into this
	int ctlPts[4];
	int *tmpInt;
	
	spot *tmpSpot;
	connection *tmpConnection;

	int firstSpot, secondSpot, x, y;
	int index = 0;
	bool run = true, drawn = true, intersects = false;
	while( run )
	{
		while( SDL_PollEvent(&event) )
		{
			switch( event.type )
			{
				case SDL_KEYUP:					// keyboard released
					if( event.key.keysym.sym == SDLK_ESCAPE )
						return false;
						// run = false;
					break;
				case SDL_MOUSEBUTTONDOWN:	break;	// mouse pressed
				case SDL_MOUSEBUTTONUP:		// mouse released
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						doLockSurface = false;
						SDL_LockSurface( surface );
							if( index == 0 )	// TODO: rewrite this using a bool for started or not started already
							{
								if( select( event.button.x, event.button.y ) )
								{
									if( spots[activeSpot]->degree < 3 )
									{
										firstSpot = activeSpot;
										tmpConnection = new connection;
										tmpConnection->xPoints.push_back( &spots[activeSpot]->xPoint );
										tmpConnection->yPoints.push_back( &spots[activeSpot]->yPoint );
										index++;
										spots[activeSpot]->degree++;
									}
								}
							}
							else
							{
								if( select( event.button.x, event.button.y ) )		// clicked last point on path
								{
									tmpConnection->xPoints.push_back( &spots[activeSpot]->xPoint );
									tmpConnection->yPoints.push_back( &spots[activeSpot]->yPoint );
									lines.push_back( tmpConnection );
									spots[activeSpot]->degree++;
									drawLines();
									run = false;
								}
								else	// clicking control points
								{
									// check if valid to add point there
									drawLines();
									if( lineValid( *tmpConnection->xPoints.back(), *tmpConnection->yPoints.back(), event.button.x, event.button.y ) )
									{
										tmpInt = new int;
										*tmpInt = event.button.x;
										tmpConnection->xPoints.push_back( tmpInt );
										tmpInt = new int;
										*tmpInt = event.button.y;
										tmpConnection->yPoints.push_back( tmpInt );
									}
								}
									drawConnection( tmpConnection );
							}
						SDL_UnlockSurface( surface );
						SDL_Flip( surface );
						doLockSurface = true;
					}
					else if( event.button.button == SDL_BUTTON_RIGHT )
					{
						if( index == 2 )
						{
							
						}
					}
					break;
				case SDL_MOUSEMOTION:		// mouse moved
					doLockSurface = false;
					SDL_LockSurface( surface );
						if( index > 0 )
						{
							drawLines();
							drawConnection( tmpConnection );
							lineValid( *tmpConnection->xPoints.back(), *tmpConnection->yPoints.back(), event.motion.x, event.motion.y );
							circleColor( surface, event.motion.x, event.motion.y, selectRadius, grayedColor );
						}
						highlightNear( event.motion.x, event.motion.y );
					SDL_UnlockSurface( surface );
					SDL_Flip( surface );
					doLockSurface = true;
					break;
				default:
					break;
			}
		}
	}
	drawLines();
	return true;
}
void Sprout::connect(int firstSpot, int secondSpot, int x, int y)
{
	int middleSpot;
	
	spot *tmpSpot;
	tmpSpot = new spot;
	connection *one, *two;
	one = new connection;
	two = new connection;

	/* Create new spot at x,y for snapping */
	tmpSpot->xPoint = x;
	tmpSpot->yPoint = y;
	tmpSpot->degree = 2;
	middleSpot = spots.size();
	spots.push_back( tmpSpot );
	drawSpots();

	/* Split line at x,y */
	one->xPoints.push_back(&spots[firstSpot ]->xPoint);
	one->xPoints.push_back(&spots[middleSpot]->xPoint);
	one->yPoints.push_back(&spots[firstSpot ]->yPoint);
	one->yPoints.push_back(&spots[middleSpot]->yPoint);
	 
	two->xPoints.push_back(&spots[secondSpot]->xPoint);
	two->xPoints.push_back(&spots[middleSpot]->xPoint);
	two->yPoints.push_back(&spots[secondSpot]->yPoint);
	two->yPoints.push_back(&spots[middleSpot]->yPoint);

	lines.push_back(two);
	lines.push_back(one);
}
void Sprout::drawSpots(void)
{
	if( doLockSurface )
		SDL_LockSurface( surface );

	for( int i = 0; i < spots.size(); i++ )
		circleColor( surface, spots[i]->xPoint, spots[i]->yPoint, spotRadius, defaultColor );

	if( doLockSurface )
	{
		SDL_UnlockSurface( surface );
		SDL_Flip( surface );
	}
}
void Sprout::drawLines(void)
{
	int xNew, yNew, xOld, yOld;
	double step;

    if( doLockSurface )
        SDL_LockSurface( surface );
	SDL_FillRect( surface, NULL, 0 );	// blank surface

	/* Draw lines */
	for( int it = 0; it < lines.size(); it++ )
	{
		// lineColor( surface, *lines[it]->xPoints[0], *lines[it]->yPoints[0], *lines[it]->xPoints[1], *lines[it]->yPoints[1], defaultColor );
		
		// iterate through each pair of control points defining the line
		for( int i = 1; i < lines[it]->xPoints.size(); i++ )	// what happens if there is only one xPoint?
		{
			// lineColor( surface, *lines[it]->xPoints[i], *lines[it]->yPoints[i], *lines[it]->xPoints[i-1], *lines[it]->yPoints[i-1], defaultColor );
			thickLine( surface, *lines[it]->xPoints[i], *lines[it]->yPoints[i], *lines[it]->xPoints[i-1], *lines[it]->yPoints[i-1], defaultColor );
		}
	}
    drawSpots();
	if( doLockSurface )
    {
        SDL_UnlockSurface( surface );
        SDL_Flip( surface );
    }
}
void Sprout::drawConnection(connection *tmpConnection)
{
	if( doLockSurface )
        SDL_LockSurface( surface );
	for( int i = 1; i < tmpConnection->xPoints.size(); i++ )	// what happens if there is only one xPoint?
	{
		// circleColor( surface, *tmpConnection->xPoints[i-1], *tmpConnection->yPoints[i-1], spotRadius/2, grayedColor );
		thickLine( surface, *tmpConnection->xPoints[i], *tmpConnection->yPoints[i], *tmpConnection->xPoints[i-1], *tmpConnection->yPoints[i-1], defaultColor );
	}
	if( doLockSurface )
    {
        SDL_UnlockSurface( surface );
        SDL_Flip( surface );
    }
}
// WARNING! only call this function if the line from index to x,y has not been drawn (only call this once per drawLines() call)
// lock the screen, blank the screen, draw the other lines, then call this function twice (for two lines), then unlock and flip the screen
bool Sprout::lineValid(int x0, int y0, int x, int y)
{
	double step;
	bool clear = true;
	int xNew, yNew, xOld, yOld;
	std::vector<int> xCollisions, yCollisions;
	/* Draw line */
	xOld = x;
	yOld = y;
	// step = 1.0 / dist( spots[index]->xPoint, spots[index]->yPoint, x, y );
	step = 1.0 / dist( x0, y0, x, y );
	for( double t = 0+step; t <= 1; t += step )
	{
		xNew = x0 * t + x * ( 1 - t );		// xNew = spots[index]->xPoint * t + x * ( 1 - t );
		yNew = y0 * t + y * ( 1 - t );		// yNew = spots[index]->yPoint * t + y * ( 1 - t );
		if( xOld == xNew && yOld == yNew )
			continue;
		if( getpixel( surface, xNew, yNew ) == SDL_MapRGBA(surface->format, 255,255,255,255) )		// pixel plotted is already there (intersecting another line)
		{
			if( dist2( xNew, yNew, x, y ) > selectRadius*selectRadius )
			{
				if( dist2( xNew, yNew, x0, y0 ) > selectRadius*selectRadius  )
				{
					xCollisions.push_back( xNew );
					yCollisions.push_back( yNew );
					// circleColor( surface, xNew, yNew, 5, highlightColor );
					clear = false;
				}
			}
		}
			
		xOld = xNew;
		yOld = yNew;
	}
	// now draw the line
	for( double t = 0+step; t <= 1; t += step )
	{
		xNew = x0 * t + x * ( 1 - t );		// xNew = spots[index]->xPoint * t + x * ( 1 - t );
		yNew = y0 * t + y * ( 1 - t );		// yNew = spots[index]->yPoint * t + y * ( 1 - t );
		{
			for( int i = -thickness; i <= thickness; i++ )
			{
				for( int j = -thickness; j <= thickness; j++ )
				{
					pixelColor(  surface, xNew+i, yNew+j, defaultColor );
				}
			}
		}
	}
	for( int it = 0; it < xCollisions.size(); it++ )
	{
		filledCircleColor( surface, xCollisions[it], yCollisions[it], 5, highlightColor );
	}
	return clear;
}
bool Sprout::select(int x, int y)
{
	if( spots.empty() )
		return false;		// no spots, so nothing to try to select
	int closestPoint;
	int closestRadius = surface->w, tmpRadius;
	for( int it = 0; it < spots.size(); it++ )
	{
		tmpRadius = dist2( x, y, spots[it]->xPoint, spots[it]->yPoint );
		if( tmpRadius < closestRadius )
		{
			closestRadius = tmpRadius;
			closestPoint = it;
		}
	}
	if( closestRadius <= selectRadius * selectRadius )
	{
		activeSpot = closestPoint;
		return true;
	}
	return false;
}
/* TODO: make this so it will draw properly if two points are too close together */
bool Sprout::highlightNear(int x, int y)
{
	if( select( x, y ) )
	{
		// if( !highlighted )	// not previously highlighted, now mouse near a spot
		{
			circleColor( surface, spots[activeSpot]->xPoint, spots[activeSpot]->yPoint, spotRadius, highlightColor );
			highlighted = true;
		}
	}
	else if( highlighted )	// was highlighted, now mouse not near that node
	{
		circleColor( surface, spots[activeSpot]->xPoint, spots[activeSpot]->yPoint, spotRadius, defaultColor );
		highlighted = false;
	}
	if( doLockSurface )
		SDL_Flip( surface );
}
inline double Sprout::dist(int x0, int y0, int x1, int y1)
{ return (sqrt((double)dist2(x0, y0, x1, y1))); }
inline int Sprout::dist2(int x0, int y0, int x1, int y1)
{ return ((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0)); }

// Note: how about this? http://www.codeproject.com/KB/graphics/BezierSpline.aspx
