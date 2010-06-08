#include <vector>
#include "draw.h"

class Sprout
{
	private:
		/* Constants */
		static const Uint32 defaultColor = 0xFFFFFFFF;		//
		static const Uint32 grayedColor = 0xD4D4D4D4;		// assuming big-endian?
		static const Uint32 highlightColor = 0xFF00FFFF;	//
		static const int sproutRadius = 5;
		static const int selectRadius = 15;
		static const int thickness = 1;		// thickness for drawing lines and points

		/* Structures and other storage stuff */
		struct sprout   { int xPoint,yPoint,degree; };
		struct connection { std::vector<int*> xPoints, yPoints; };

		std::vector<sprout*>  sprouts;	// all the sprouts
		std::vector<connection*> lines;	// all the lines

		int activeSprout;

		/* Variables */
		SDL_Surface *surface;	// screen for drawing
		bool doLockSurface;		// for internal use - make this false (be sure to reset it) if multiple draw commands need to be executed before flipping the surface
		bool highlighted;

		/* Private functions */
		void thickLine(SDL_Surface*,int,int,int,int,Uint32);	// draws a line of thickness n+2 (last parameter n)
		  void connect(int,int,int,int);	// (by index) connect first sprout to second sprout through (x,y)
		  void drawSprouts(void);		// draw all the sprouts
		  void drawConnection(connection*);		// draw the current temporary line
		  bool select(int,int);			// set sprout active it near x,y
		double dist(int,int,int,int);	// return distance between (x0,y0) and (x1,y1)
		   int dist2(int,int,int,int);	// return square of distance between (x0,y0) and (x1,y1)
		  bool lineValid(int,int,int,int);	// draw line segments from x0,y0 to x,y : returns false if hit another line
	public:
		Sprout(SDL_Surface*,int);	// default constructor : feed it a surface and initial sprouts number

        void drawLines(void);
		bool highlightNear(int,int);	// highlight sprout if near x,y (calls select())
		bool connect(void);				// loop to connect two sprouts
};
Sprout::Sprout(SDL_Surface *sf, int numSprouts)
{
	doLockSurface = true;
	highlighted = false;
	surface = sf;
	double theta = 0;

	/* Set up initial board (seed sprouts) */
	sprout *tmpSprout;
	
	/* Make first sprout centered */
	tmpSprout = new sprout;
	tmpSprout->degree = 0;
	tmpSprout->xPoint = surface->w/2;
	tmpSprout->yPoint = surface->h/2;
	sprouts.push_back( tmpSprout );
	
	/* Put sprouts along an ellipse at the center of the screen */
	for( int i = 1; i < numSprouts; i++ )
	{
		tmpSprout = new sprout;
		tmpSprout->degree = 0;

		tmpSprout->xPoint = surface->w/3 * cos(theta) + surface->w/2;
		tmpSprout->yPoint = surface->h/3 * sin(theta) + surface->h/2;
		theta += 2*3.14 / numSprouts;

		sprouts.push_back( tmpSprout );
	}
	drawSprouts();
	/* End sprouts setup */
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
	
	int firstSprout, secondSprout, x, y;
	int index = 0;
	int xVec, yVec;
	double xUnit, yUnit;
	bool run = true, drawn = true, intersects = false, planted = false;

	sprout *tmpSprout;
	connection *tmpConnection;

	while( run )
	{
		while( SDL_PollEvent(&event) )
		{
			switch( event.type )
			{
				case SDL_KEYUP:					// keyboard released
					if( event.key.keysym.sym == SDLK_ESCAPE )
					{
						drawLines();
						return true;	// true?
					}
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
									if( sprouts[activeSprout]->degree < 3 )
									{
										firstSprout = activeSprout;
										tmpConnection = new connection;
										tmpConnection->xPoints.push_back( &sprouts[activeSprout]->xPoint );
										tmpConnection->yPoints.push_back( &sprouts[activeSprout]->yPoint );
										index++;
										sprouts[activeSprout]->degree++;
									}
								}
							}
							else
							{
								if( select( event.button.x, event.button.y ) && planted )		// clicked last point on path
								{
									if( sprouts[activeSprout]->degree < 3 )
									{
										tmpConnection->xPoints.push_back( &sprouts[activeSprout]->xPoint );
										tmpConnection->yPoints.push_back( &sprouts[activeSprout]->yPoint );
										lines.push_back( tmpConnection );
										sprouts[activeSprout]->degree++;
										drawLines();
										run = false;
									}
								}
								else	// clicking control points
								{
									// check if valid to add point there
									drawLines();
									drawConnection( tmpConnection );
									if( lineValid( *tmpConnection->xPoints.back(), *tmpConnection->yPoints.back(), event.button.x, event.button.y ) )
									{
										if( select( event.button.x, event.button.y ) )	// mouse too close to another node
										{
											xVec = event.button.x - sprouts[activeSprout]->xPoint;
											yVec = event.button.y - sprouts[activeSprout]->yPoint;

											xUnit = xVec / ( sqrt( xVec*xVec + yVec*yVec) );
											yUnit = yVec / ( sqrt( xVec*xVec + yVec*yVec) );

											xVec = xUnit * selectRadius * 1.5;
											yVec = yUnit * selectRadius * 1.5;

											SDL_WarpMouse( event.button.x+xVec, event.button.y+yVec );	// push mouse away from that node
										}
										else
										{
											tmpInt = new int;
											*tmpInt = event.button.x;
											tmpConnection->xPoints.push_back( tmpInt );
											tmpInt = new int;
											*tmpInt = event.button.y;
											tmpConnection->yPoints.push_back( tmpInt );
										}
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
						if( index > 0 )
						{
							if( !planted )
							{
								doLockSurface = false;
								SDL_LockSurface( surface );
									drawLines();
									drawConnection( tmpConnection );
									if( lineValid( *tmpConnection->xPoints.back(), *tmpConnection->yPoints.back(), event.button.x, event.button.y ) )
									{
										tmpSprout = new sprout;
										tmpSprout->xPoint = event.button.x;
										tmpSprout->yPoint = event.button.y;
										tmpSprout->degree = 2;
										sprouts.push_back( tmpSprout );
										tmpConnection->xPoints.push_back( &sprouts.back()->xPoint );
										tmpConnection->yPoints.push_back( &sprouts.back()->yPoint );
										drawConnection( tmpConnection );
										planted = true;
									}
									drawLines();
									drawConnection( tmpConnection );
								SDL_UnlockSurface( surface );
								SDL_Flip( surface );
								doLockSurface = true;
							}
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
	/* Check if all nodes have degree 3 */
	int twos = 0;
	for( int i = 0; i < sprouts.size(); i++ )
	{
		if( sprouts[i]->degree == 2 )
		{
			twos++;
		}
		
		if( sprouts[i]->degree == 1 )	// still nodes left to connect
		{
			i = sprouts.size();
			twos = 0;
		}
	}
	if( twos == 1 )
		return false;
	return true;
}
void Sprout::connect(int firstSprout, int secondSprout, int x, int y)
{
	int middleSprout;

	sprout *tmpSprout;
	tmpSprout = new sprout;
	connection *one, *two;
	one = new connection;
	two = new connection;

	/* Create new sprout at x,y for snapping */
	tmpSprout->xPoint = x;
	tmpSprout->yPoint = y;
	tmpSprout->degree = 2;
	middleSprout = sprouts.size();
	sprouts.push_back( tmpSprout );
	drawSprouts();

	/* Split line at x,y */
	one->xPoints.push_back(&sprouts[firstSprout ]->xPoint);
	one->xPoints.push_back(&sprouts[middleSprout]->xPoint);
	one->yPoints.push_back(&sprouts[firstSprout ]->yPoint);
	one->yPoints.push_back(&sprouts[middleSprout]->yPoint);

	two->xPoints.push_back(&sprouts[secondSprout]->xPoint);
	two->xPoints.push_back(&sprouts[middleSprout]->xPoint);
	two->yPoints.push_back(&sprouts[secondSprout]->yPoint);
	two->yPoints.push_back(&sprouts[middleSprout]->yPoint);

	lines.push_back(two);
	lines.push_back(one);
}
void Sprout::drawSprouts(void)
{
	if( doLockSurface )
		SDL_LockSurface( surface );

	for( int i = 0; i < sprouts.size(); i++ )
		circleColor( surface, sprouts[i]->xPoint, sprouts[i]->yPoint, sproutRadius, grayedColor );

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
		// iterate through each pair of control points defining the line
		for( int i = 1; i < lines[it]->xPoints.size(); i++ )	// what happens if there is only one xPoint?
		{
			thickLine( surface, *lines[it]->xPoints[i], *lines[it]->yPoints[i], *lines[it]->xPoints[i-1], *lines[it]->yPoints[i-1], defaultColor );
		}
	}
    drawSprouts();
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
	/* Check line segment between x0,y0 and x,y */
	xOld = x;
	yOld = y;
	step = 1.0 / dist( x0, y0, x, y );
	for( double t = 0+step; t <= 1; t += step )
	{
		xNew = x0 * t + x * ( 1 - t );		// xNew = sprouts[index]->xPoint * t + x * ( 1 - t );
		yNew = y0 * t + y * ( 1 - t );		// yNew = sprouts[index]->yPoint * t + y * ( 1 - t );
		if( xOld == xNew && yOld == yNew )
			continue;
		if( getpixel( surface, xNew, yNew ) == SDL_MapRGBA(surface->format, 255,255,255,255) )		// pixel plotted is already there (intersecting another line)
		{
			// if( dist2( xNew, yNew, x, y ) > selectRadius*selectRadius )
			{
				if( dist2( xNew, yNew, x0, y0 ) > sproutRadius*sproutRadius )//selectRadius*selectRadius  )
				{
					xCollisions.push_back( xNew );
					yCollisions.push_back( yNew );
					clear = false;
				}
			}
		}

		xOld = xNew;
		yOld = yNew;
	}
	/* Draw line */
	for( double t = 0+step; t <= 1; t += step )
	{
		xNew = x0 * t + x * ( 1 - t );		// xNew = sprouts[index]->xPoint * t + x * ( 1 - t );
		yNew = y0 * t + y * ( 1 - t );		// yNew = sprouts[index]->yPoint * t + y * ( 1 - t );
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
	/* Draw circles at collisions */
	for( int it = 0; it < xCollisions.size(); it++ )
	{
		filledCircleColor( surface, xCollisions[it], yCollisions[it], 5, highlightColor );
	}
	return clear;
}
bool Sprout::select(int x, int y)
{
	if( sprouts.empty() )
		return false;		// no sprouts, so nothing to try to select
	int closestPoint, closestRadius = surface->w, tmpRadius;
	for( int it = 0; it < sprouts.size(); it++ )
	{
		tmpRadius = dist2( x, y, sprouts[it]->xPoint, sprouts[it]->yPoint );
		if( tmpRadius < closestRadius )
		{
			closestRadius = tmpRadius;
			closestPoint = it;
		}
	}
	if( closestRadius <= selectRadius * selectRadius )		// found a sprout withing selectRadius of x,y
	{
		activeSprout = closestPoint;
		return true;
	}
	return false;
}
/* TODO: make this so it will draw properly if two points are too close together */
bool Sprout::highlightNear(int x, int y)
{
	if( select( x, y ) )
	{
		// if( !highlighted )	// not previously highlighted, now mouse near a sprout
		{
			circleColor( surface, sprouts[activeSprout]->xPoint, sprouts[activeSprout]->yPoint, sproutRadius, highlightColor );
			highlighted = true;
		}
	}
	else if( highlighted )	// was highlighted, now mouse not near that node
	{
		circleColor( surface, sprouts[activeSprout]->xPoint, sprouts[activeSprout]->yPoint, sproutRadius, grayedColor );
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
