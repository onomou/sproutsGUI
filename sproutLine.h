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
		// static const int curvePoints = 30;

		/* Structures and other storage stuff */
		struct spot   { int xPoint,yPoint,degree; };
		// struct bezier { int *xPoints[3],*yPoints[3]; };
		struct connection { std::vector<int*> xPoints, yPoints; };

		std::vector<spot*>  spots;	// all the spots
		std::vector<connection*> lines;	// all the lines

		int activeSpot;

		struct bucket
		{
			std::vector<int> edgePoints;
			std::vector<bucket> interior;
		};
		std::vector<bucket> base;



		/* Variables */
		SDL_Surface *surface;	// screen for drawing
		bool doLockSurface;		// for internal use - make this false (be sure to reset it) if multiple draw commands need to be executed before flipping the surface
		bool highlighted;

		/* Private functions */
		  void connect(int,int,int,int);	// (by index) connect first spot to second spot through (x,y)
		  void refine(int,int,int);		// add more control points for a given line
		  void drawSpots(void);
		  bool select(int,int);			// set spot active it near x,y
		double dist(int,int,int,int);	// return distance between (x0,y0) and (x1,y1)
		   int dist2(int,int,int,int);	// return square of distance between (x0,y0) and (x1,y1)
		  bool lineValid(connection*,int,int);	// draw line segments from index to x,y : returns false if hit another line
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
bool Sprout::connect(void)
{
	SDL_Event event;			// dump event polls into this
	int ctlPts[4];
	
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
						run = false;
					break;
				case SDL_MOUSEBUTTONDOWN:	break;	// mouse pressed
				case SDL_MOUSEBUTTONUP:		// mouse released
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						switch( index )
						{
							case 0:
								if( select( event.button.x, event.button.y ) )
								{
									firstSpot = activeSpot;
									index++;
								}
								break;	// now index is 1
							case 1:
								if( select( event.button.x, event.button.y ) )
								{
									secondSpot = activeSpot;
									index++;
									
									/* Make a new spot under mouse : next click will fix its location */
									tmpSpot = new spot;
									tmpSpot->xPoint = event.button.x;
									tmpSpot->yPoint = event.button.y;
									/* Make a new line (that can be refined) stuck to these three points */
									tmpConnection = new connection;
									tmpConnection->xPoints.push_back( &spots[firstSpot ]->xPoint );
									tmpConnection->xPoints.push_back( &spots[secondSpot]->xPoint );
									tmpConnection->yPoints.push_back( &spots[firstSpot ]->yPoint );
									tmpConnection->yPoints.push_back( &spots[secondSpot]->yPoint );
								}
								break;	// now index is 2
							case 2:
								x = event.button.x;
								y = event.button.y;
								drawLines();
								
								if( lineValid( tmpConnection, x, y ) && lineValid( tmpConnection, x, y ) )		// hit a line in between
								{
									/* Fix new spot at current mouse location */
									tmpSpot->xPoint = event.button.x;
									tmpSpot->yPoint = event.button.y;
									spots.push_back( tmpSpot );
									/* Connect the temporary line to the new spot */
									tmpConnection->xPoints.insert( tmpConnection->xPoints.end()-1, &spots.back()->xPoint );
									tmpConnection->yPoints.insert( tmpConnection->yPoints.end()-1, &spots.back()->yPoint );
									lines.push_back( tmpConnection );
									
									// connect( firstSpot, secondSpot, x, y );
									drawLines();
									run = false;
								}
								break;
							default:
								run = false;
						}
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
					drawLines();
					highlightNear( event.motion.x, event.motion.y );

					switch( index )		// this switch statement cascades 2,1,0 (could be written more succinctly)
					{
						case 0:
						case 1:
							circleColor( surface, event.motion.x, event.motion.y, selectRadius, grayedColor );
							break;
						case 2:
							lineValid( tmpConnection, event.motion.x, event.motion.y );
							circleColor( surface, event.motion.x, event.motion.y, selectRadius, grayedColor );
							break;
/*						case 2:
							circleColor( surface, event.motion.x, event.motion.y, spotRadius, highlightColor );
							// lineValid(  firstSpot, event.motion.x, event.motion.y );
							// lineValid( secondSpot, event.motion.x, event.motion.y );
							lineValid( lineIndex, event.motion.x, event.motion.y
							break;
*/						default:
							break;
					}
					SDL_UnlockSurface( surface );
					SDL_Flip( surface );
					break;
				default:
					break;
			}
		}
	}
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
void Sprout::refine(int firstSpot, int x, int y)
{
	int *xNew, *yNew;
	xNew = new int;
	yNew = new int;
	*xNew = x;
	*yNew = y;
	lines.back()->xPoints.insert( lines.back()->xPoints.begin()+1, xNew );
	lines.back()->yPoints.insert( lines.back()->yPoints.begin()+1, yNew );
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
			lineColor( surface, *lines[it]->xPoints[i], *lines[it]->yPoints[i], *lines[it]->xPoints[i-1], *lines[it]->yPoints[i-1], defaultColor );
		}
	}
    drawSpots();
	if( doLockSurface )
    {
        SDL_UnlockSurface( surface );
        SDL_Flip( surface );
    }
}
// WARNING! only call this function if the line from index to x,y has not been drawn (only call this once per drawLines() call)
// lock the screen, blank the screen, draw the other lines, then call this function twice (for two lines), then unlock and flip the screen
bool Sprout::lineValid(connection *lin, int x, int y)
{
	double step;
	bool clear = true;
	int xNew, yNew, xOld, yOld;
	/* Draw line */
	xOld = x;
	yOld = y;
	// step = 1.0 / dist( spots[index]->xPoint, spots[index]->yPoint, x, y );
	step = 1.0 / dist( *lin->xPoints[lin->xPoints.size()-1], *lin->yPoints[lin->yPoints.size()-1], x, y );	//.back()
	for( double t = 0+step; t <= 1; t += step )
	{
		xNew = *lin->xPoints.back() * t + x * ( 1 - t );		// xNew = spots[index]->xPoint * t + x * ( 1 - t );
		yNew = *lin->yPoints.back() * t + y * ( 1 - t );		// yNew = spots[index]->yPoint * t + y * ( 1 - t );
		if( xOld == xNew && yOld == yNew )
			continue;
		if( getpixel( surface, xNew, yNew ) == SDL_MapRGBA(surface->format, 255,255,255,255) )		// pixel plotted is already there (intersecting another line)
		{
			if( dist2( xNew, yNew, x, y ) > selectRadius*selectRadius )
			{
				if( dist2( xNew, yNew, *lin->xPoints.back(), *lin->yPoints.back() > selectRadius*selectRadius ) )	// if( dist2( xNew, yNew, spots[index]->xPoint, spots[index]->yPoint ) > selectRadius*selectRadius )
				{
					circleColor( surface, xNew, yNew, 5, highlightColor );
					clear = false;
				}
			}
		}
		else
			pixelColor( surface, xNew, yNew, defaultColor );
			
		xOld = xNew;
		yOld = yNew;
	}
	if( !clear )
		circleColor( surface, 10,10,10,grayedColor);
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
