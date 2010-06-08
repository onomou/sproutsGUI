#include <vector>
#include "xorRNG.h"
#include "graph.h"

class Sprout
{
	private:
		/* Constants */
		static const Uint32 defaultColor = 0xFFFFFFFF;		//
		static const Uint32 grayedColor = 0xD4D4D480;		// assuming big-endian?
		static const Uint32 highlightColor = 0xFF00FFFF;	//
		static const int spotRadius = 5;
		static const int selectRadius = 15;
		static const int curvePoints = 30;

		/* Structures and other storage stuff */
		struct spot   { int xPoint,yPoint,degree; };
		struct bezier { int *xPoints[3],*yPoints[3]; };
		
		std::vector<spot*>  spots;	// all the spots
		std::vector<bezier> lines;	// all the lines

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
		  void drawSpots(void);
		  bool select(int,int);			// set spot active it near x,y
		double dist(int,int,int,int);	// return distance between (x0,y0) and (x1,y1)
		   int dist2(int,int,int,int);	// return square of distance between (x0,y0) and (x1,y1)
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

	int firstSpot, secondSpot, x, y;
	int index = 0;
	bool run = true, drawn = true;
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
								break;
							case 1:
								if( select( event.button.x, event.button.y ) )
								{
									secondSpot = activeSpot;
									index++;
								}
								break;
							case 2:
								x = event.button.x;
								y = event.button.y;
								connect( firstSpot,secondSpot,x,y );
							default:
								run = false;
						}
					}
					break;
				case SDL_MOUSEMOTION:		// mouse moved
					highlightNear( event.motion.x, event.motion.y );
					break;
				default:
					break;
			}
		}
	}
}
void Sprout::connect(int firstSpot, int secondSpot, int x, int y)
{
	bezier first,second;
	spot *tmpSpot;
	tmpSpot = new spot;
	int middleSpot;
	int Ax,Ay,Bx,By,QoX,QoY;
	double Cx,Cy;

	int xNew, yNew;
	double step = 1.0/curvePoints;
	double  smallestDistance = dist2(x,y,spots[firstSpot]->xPoint,spots[firstSpot]->yPoint),
			tClosest;

	/* Get initial curve control points */
	int p0x = spots[firstSpot]->xPoint;
	int p0y = spots[firstSpot]->yPoint;
	int p1x = spots[secondSpot]->xPoint;
	int p1y = spots[secondSpot]->yPoint;
	int p2x = x;
	int p2y = y;
	int v01x = p1x - p0x;
	int v01y = p1y - p0y;
	int v02x = p2x - p0x;
	int v02y = p2y - p0y;
	int v01p02x = double(v01x*v02x + v01y*v02y)/double(v01x*v01x + v01y*v01y)*v01x;	// projection of p02 on p01
	int v01p02y = double(v01x*v02x + v01y*v02y)/double(v01x*v01x + v01y*v01y)*v01y;	// projection of p02 on p01

	int xControl = 2 * p2x - (p0x + v01p02x);
	int yControl = 2 * p2y - (p0y + v01p02y);

	/* Find t value at which distance to x,y is minimum */
	for( double t = 0; t <= 1; t += step )
	{
		xNew = (1-t)*(1-t)*(spots[firstSpot]->xPoint) + 2*(1-t)*t*xControl + t*t*(spots[secondSpot]->xPoint);
		yNew = (1-t)*(1-t)*(spots[firstSpot]->yPoint) + 2*(1-t)*t*yControl + t*t*(spots[secondSpot]->yPoint);

		// pixelColor( surface, xNew, yNew, highlightColor );
		SDL_Flip( surface );
		if( dist2(x,y,xNew,yNew) < smallestDistance )	// distance from given (x,y) to curve (x,y) is within 2 pixels
		{
			smallestDistance = dist2( x,y, xNew,yNew );
			std::cout << smallestDistance << std::endl;
			if( smallestDistance == 0 )
				break;
			tClosest = t;
		}
	}

	/* Create new spot at x,y for snapping */
	// tmpSpot.xPoint = x;
	// tmpSpot.yPoint = y;
	tmpSpot->xPoint = (1-tClosest)*(1-tClosest)*(spots[firstSpot]->xPoint) + 2*(1-tClosest)*tClosest*xControl + tClosest*tClosest*(spots[secondSpot]->xPoint);
	tmpSpot->yPoint = (1-tClosest)*(1-tClosest)*(spots[firstSpot]->yPoint) + 2*(1-tClosest)*tClosest*yControl + tClosest*tClosest*(spots[secondSpot]->yPoint);
	tmpSpot->degree = 2;
	middleSpot = spots.size();
	spots.push_back( tmpSpot );
	drawSpots();

	/* Split line at x,y */
	  first.xPoints[1] = new int;
	  first.yPoints[1] = new int;
	 second.xPoints[1] = new int;
	 second.yPoints[1] = new int;

	  first.xPoints[0] = &spots[firstSpot]->xPoint;
	 *first.xPoints[1] = (1-tClosest)*(spots[firstSpot]->xPoint) + tClosest*xControl;
	  first.xPoints[2] = &spots[middleSpot]->xPoint;
	  first.yPoints[0] = &spots[firstSpot]->yPoint;
	 *first.yPoints[1] = (1-tClosest)*(spots[firstSpot]->yPoint) + tClosest*yControl;
	  first.yPoints[2] = &spots[middleSpot]->yPoint;

	 second.xPoints[0] = &spots[middleSpot]->xPoint;
	*second.xPoints[1] = (1-tClosest)*xControl + tClosest*(spots[secondSpot]->xPoint);
	 second.xPoints[2] = &spots[secondSpot]->xPoint;
	 second.yPoints[0] = &spots[middleSpot]->yPoint;
	*second.yPoints[1] = (1-tClosest)*yControl + tClosest*(spots[secondSpot]->yPoint);
	 second.yPoints[2] = &spots[secondSpot]->yPoint;

	lines.push_back(first);
	lines.push_back(second);
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
	double step = 1.0/curvePoints;

    if( doLockSurface )
        SDL_LockSurface( surface );
	SDL_FillRect( surface, NULL, 0 );	// clear surface to black
	for( int it = 0; it < lines.size(); it++ )
	{
		lineColor( surface, *lines[it].xPoints[0], *lines[it].yPoints[0], *lines[it].xPoints[1], *lines[it].yPoints[1], grayedColor );
		lineColor( surface, *lines[it].xPoints[0], *lines[it].yPoints[0], *lines[it].xPoints[2], *lines[it].yPoints[2], grayedColor );
		lineColor( surface, *lines[it].xPoints[1], *lines[it].yPoints[1], *lines[it].xPoints[2], *lines[it].yPoints[2], grayedColor );

		/* Draw Bezier curve for current line */
		xOld = *lines[it].xPoints[0];
		yOld = *lines[it].yPoints[0];
		for( double t = 0+step, points = 0; points < curvePoints; t += step, points++ )
		{
			xNew = (1-t)*(1-t)* *lines[it].xPoints[0] + 2*(1-t)*t* *lines[it].xPoints[1] + t*t* *lines[it].xPoints[2];
			yNew = (1-t)*(1-t)* *lines[it].yPoints[0] + 2*(1-t)*t* *lines[it].yPoints[1] + t*t* *lines[it].yPoints[2];

			// for( int i = -2; i <= 2; i++ )
				// for( int j = -2; j <= 2; j++ )
					// lineColor( picking, xNew+i, yNew+j, xOld+i, yOld+j, defaultColor );

			lineColor( surface, xNew, yNew, xOld, yOld, defaultColor );

			xOld = xNew;
			yOld = yNew;
		}
	}
    drawSpots();
	if( doLockSurface )
    {
        SDL_UnlockSurface( surface );
        SDL_Flip( surface );
    }
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
		if( !highlighted )	// not previously highlighted, now mouse near a spot
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
