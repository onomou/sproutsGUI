#include <algorithm>
#include <ctime>
#include <vector>
#include <cmath>
#include "xorRNG.h"
#include "draw.h"


class Bezier
{
	private:

	struct t
	{
		int xCenter, yCenter, lineIndex;
	};
	struct v
	{
		int xRel, yRel, xOrigin, yOrigin, lineIndex;
	};

		/* Global constants for this class  */
		static const int r = 255,
						 g = 255,
						 b = 255,
						 a = 255;
		static const int curvePoints = 40;
		static const int circleRadius = 5;
		static const int radiusGlobal = 15;
		static const int radiusRadius = 225;
		static const Uint32 defaultColor = 0xFFFFFFFF;		//
		static const Uint32 grayedColor = 0xD4D4D480;		// assuming big-endian?
		static const Uint32 highlightColor = 0xFF00FFFF;	//
		/* End constants */

		struct pointsLines					// for associating an active line, point pair and its distance from some other point
		{ int aLine, aPoint, dist; };

		struct bLine
		{
			int *xPoints[4];				// array of 4 pointers to integers
			int *yPoints[4];				//
			int activePoint;
		};
		std::vector<bLine> allLines;
		int activeLine;

		SDL_Surface *surface;				// screen to draw onto
		SDL_Surface *picking;	// may be used for selecting nodes at some point (if checking all distances becomes too slow) : NOT USED CURRENTLY

		bool doLockSurface;		// for internal use - make this false (be sure to reset it) if multiple draw commands need to be executed before flipping the surface
		bool highlighted;		// true iff mouse is near a point

		/* Private functions */
		double dist(int,int,int,int);		// return distance between (x,y) and (x1,y1)
		double distLine(int,int,int,int);	// return distance between (x,y) and (lineIndex,pointIndex)
		void drawCircles(bLine&,Uint32=defaultColor);	// draw curve's circles, default color white

	public:
		/* Public variables */
		bool active;					// true if moving a point

		/* Constructors */
		Bezier(SDL_Surface*);			// default constructor

		/* Curve generation */
		void addLine(bool=false);		// make new line with user input, or random point positions if bool is true

		/* Curve selection */
		void highlightNear(int,int);	// highlights a node if input near enough to select it - call drawLines() first
		bool select(int,int,bool=false);	// accepts x,y : if close to a point, set that point active - bool value controls whether to select the active point
		bool select(int,int,int,int);	// accepts x,y and oldLine,oldPoint to skip an already found point

		/* Curve operations */
		void move(int,int);				// move active point to (x,y)
		bool connect(int,int);			// connects the point at (x,y) to an existing point (if near enough)
		bool disconnect(int,int);		// disconnect points near (x,y) (if near enough - looks for near node)
		bool splitLine(void);			// accepts input (endpoint,endpoint) and spot on curve to split it into two
		void splitLine(bLine,int,int);

		void activateLine(void);
		void activateLine2(void);

		/* Curve visualization */
		void drawLines(Uint32=defaultColor);	// blank surface, then draw all lines in the structure - default color is white
		void drawLine(bLine);			// blank surface, then draw only given line
};
Bezier::Bezier(SDL_Surface *sf)
{
	srand( time(NULL) );

	/* Set drawing surface */
	surface = sf;
	// picking = SDL_ConvertSurface(sf, sf->format, sf->flags);	// TODO: look into using a picking layer to select nodes/find close nodes/entc - NOW: copy current video surface
	// SDL_FillRect( picking, NULL, 0 );							// blank picking surface

	/* Generate a new curve */
	// addLine(true);

	active = false;		// not moving a point initially
	doLockSurface = true;
	highlighted = false;
}
void Bezier::addLine(bool rnd)
{
	bool run = true;
	int xMouse, yMouse, pointBackup;
	bLine tmpBezier;
	SDL_Event event;
	SDL_PollEvent(&event);
	if( rnd )	// TODO: Make this part flow better so it isn't completely separate from the following section
	{
		for( int i = 0; i < 4; i++ )
		{
			 tmpBezier.xPoints[i] = new int;
			 tmpBezier.yPoints[i] = new int;
			*tmpBezier.xPoints[i] = random(surface->w);	// pick random points within the surface area
			*tmpBezier.yPoints[i] = random(surface->h); //
		}
		allLines.push_back(tmpBezier);
	}
	else
	{
		for( int i = 0; i < 4; i++ )
		{
			tmpBezier.xPoints[i] = new int;
			tmpBezier.yPoints[i] = new int;
			*tmpBezier.xPoints[i] = event.button.x;		// initialize all points to current mouse location
			*tmpBezier.yPoints[i] = event.button.y;
		}
		tmpBezier.activePoint = 0;
		activeLine = allLines.size();
		allLines.push_back(tmpBezier);

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
}
void Bezier::highlightNear(int x, int y)
{
	if( select(x,y) )
	{
		if( !highlighted )	// not previously highlighted, now mouse near a node
		{
			circleColor( surface, *allLines[activeLine].xPoints[allLines[activeLine].activePoint], *allLines[activeLine].yPoints[allLines[activeLine].activePoint], circleRadius, highlightColor );
			highlighted = true;
		}
	}
	else if( highlighted )	// was highlighted, now mouse not near that node
	{
		circleColor( surface, *allLines[activeLine].xPoints[allLines[activeLine].activePoint], *allLines[activeLine].yPoints[allLines[activeLine].activePoint], circleRadius, defaultColor );
		highlighted = false;
	}
	if( doLockSurface )
		SDL_Flip( surface );
}
// http://www.libsdl.org/intro.en/usingvideo.html
bool Bezier::select(int x, int y, bool adding)
{
	int d;	// square of radius
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
	int d;	// (square of radius)
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
void Bezier::move(int x, int y)
{
	if( active )
	{
		*allLines[activeLine].xPoints[allLines[activeLine].activePoint] = x;
		*allLines[activeLine].yPoints[allLines[activeLine].activePoint] = y;
	}
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
/* TODO: Either disconnect all nodes linked at the given coordinates, or something. Think about it. */
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
bool Bezier::splitLine(void)
{
	int selectedPoints = 0, firstLine, firstPoint, secondLine, secondPoint;
	int oldPoint, oldLine, searchBounds = 3;
	bool run = true, selected = false;
	bLine bl;
	SDL_Event event;
	drawLines( grayedColor );
	while( run )
	{
		while( SDL_PollEvent(&event) )		// get events
		{
			switch( event.type )
			{
				case SDL_MOUSEBUTTONDOWN:	// mouse pressed
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						// the select function needs some help - if two lines are connected to a single node, it will just select an arbitrary line (which one should it choose?) - maybe click on node, then click on line from node
						if( select( event.button.x, event.button.y, false) )		// clicked near another node : snap to that node	(WARNING! This function will change activeLine and activePoint if it returns true)
						{
							selected = true;
							bl = allLines[activeLine];	// copy chosen line to a temporary one
							drawLines( grayedColor );
							drawLine( bl );
							allLines.erase(allLines.begin() + activeLine);	// remove line to be split (it is now in bl)
							run = false;
						}
						break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if( event.button.button == SDL_BUTTON_RIGHT )
						return false;
					break;
				case SDL_MOUSEMOTION:
						// fill in here with code to highlight a near node (working on it)
					doLockSurface = false;
					SDL_LockSurface( surface );
						/* TODO: make select() and highlight() functions work together so select() won't be called twice in this section */
						if( select( event.motion.x, event.motion.y ) )		// found a node near the mouse - highlight that line
						{
							drawLines( grayedColor );
							drawLine( allLines[activeLine] );
						}
						highlightNear( event.motion.x, event.motion.y );
					SDL_UnlockSurface( surface );
					SDL_Flip( surface );
					doLockSurface = true;
					break;
				case SDL_KEYUP:				// keyboard released
					if( event.key.keysym.sym == SDLK_ESCAPE )
						return false;	// break out of splitting - this should reinstate the chosen line (to be done later)
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
						for( int i = -searchBounds; i <= searchBounds; i++ )
						{
							for( int j = -searchBounds; j <= searchBounds; j++ )
							{
								if( (i-j)*(i-j) > searchBounds*searchBounds )	// exclude edges of i X j square not in a circle (huh?)
									continue;
								if( getpixel( surface, event.button.x + i, event.button.y + j ) == defaultColor )	// found white pixel (curve point probably)
								{
									splitLine( bl, event.button.x + i, event.button.y + j );
									drawLines();
									return true;
								}
					}	}	}		// what about this notation for ending a set of brackets? heh heh...
					break;
				case SDL_MOUSEBUTTONUP:
				case SDL_KEYUP:
					if( event.button.button == SDL_BUTTON_RIGHT || event.key.keysym.sym == SDLK_ESCAPE )
					{
						allLines.push_back( bl );
						return false;
					}
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
	double smallestDistance = dist( x,y, *bl.xPoints[0],*bl.yPoints[0] ), smallestT;
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
	// find t value at which distance to target is minimum
	for( double t = 0; t <= 1; t += step )
	{
		xNew = t*t*t*( *bl.xPoints[3] - *bl.xPoints[0] + 3*( *bl.xPoints[1] - *bl.xPoints[2] ) ) + 3*t*t*( *bl.xPoints[0] - 2**bl.xPoints[1] + *bl.xPoints[2] ) - 3*t*( *bl.xPoints[0] - *bl.xPoints[1] ) + ( *bl.xPoints[0] );
		yNew = t*t*t*( *bl.yPoints[3] - *bl.yPoints[0] + 3*( *bl.yPoints[1] - *bl.yPoints[2] ) ) + 3*t*t*( *bl.yPoints[0] - 2**bl.yPoints[1] + *bl.yPoints[2] ) - 3*t*( *bl.yPoints[0] - *bl.yPoints[1] ) + ( *bl.yPoints[0] );

		if( dist(x,y,xNew,yNew) < smallestDistance )	// distance from given (x,y) to curve (x,y) is within 2 pixels
		{
			smallestDistance = dist( x,y, xNew,yNew );
			smallestT = t;
		}
	}
	// now split the curve
	 *first.xPoints[0] = *bl.xPoints[0];
	 *first.xPoints[1] = (1-smallestT)* *bl.xPoints[0]+smallestT* *bl.xPoints[1];
	 *first.xPoints[2] = (1-smallestT)*((1-smallestT)* *bl.xPoints[0]+smallestT* *bl.xPoints[1])+smallestT*((1-smallestT)* *bl.xPoints[1]+smallestT* *bl.xPoints[2]);
	 *first.xPoints[3] = (1-smallestT)*((1-smallestT)*((1-smallestT)* *bl.xPoints[0]+smallestT* *bl.xPoints[1])+smallestT*((1-smallestT)* *bl.xPoints[1]+smallestT* *bl.xPoints[2]))+smallestT*((1-smallestT)*((1-smallestT)* *bl.xPoints[1]+smallestT* *bl.xPoints[2])+smallestT*((1-smallestT)* *bl.xPoints[2]+smallestT* *bl.xPoints[3]));
	 *first.yPoints[0] = *bl.yPoints[0];
	 *first.yPoints[1] = (1-smallestT)* *bl.yPoints[0]+smallestT* *bl.yPoints[1];
	 *first.yPoints[2] = (1-smallestT)*((1-smallestT)* *bl.yPoints[0]+smallestT* *bl.yPoints[1])+smallestT*((1-smallestT)* *bl.yPoints[1]+smallestT* *bl.yPoints[2]);
	 *first.yPoints[3] = (1-smallestT)*((1-smallestT)*((1-smallestT)* *bl.yPoints[0]+smallestT* *bl.yPoints[1])+smallestT*((1-smallestT)* *bl.yPoints[1]+smallestT* *bl.yPoints[2]))+smallestT*((1-smallestT)*((1-smallestT)* *bl.yPoints[1]+smallestT* *bl.yPoints[2])+smallestT*((1-smallestT)* *bl.yPoints[2]+smallestT* *bl.yPoints[3]));

	 second.xPoints[0] = first.xPoints[3];	// connect midpoint
	 second.yPoints[0] = first.yPoints[3];	//

	*second.xPoints[1] = (1-smallestT)*((1-smallestT)* *bl.xPoints[1]+smallestT* *bl.xPoints[2])+smallestT*((1-smallestT)* *bl.xPoints[2]+smallestT* *bl.xPoints[3]);
	*second.xPoints[2] = (1-smallestT)* *bl.xPoints[2]+smallestT* *bl.xPoints[3];
	*second.xPoints[3] = *bl.xPoints[3];
	*second.yPoints[1] = (1-smallestT)*((1-smallestT)* *bl.yPoints[1]+smallestT* *bl.yPoints[2])+smallestT*((1-smallestT)* *bl.yPoints[2]+smallestT* *bl.yPoints[3]);
	*second.yPoints[2] = (1-smallestT)* *bl.yPoints[2]+smallestT* *bl.yPoints[3];
	*second.yPoints[3] = *bl.yPoints[3];
	allLines.push_back( first );
	allLines.push_back( second );
}
void Bezier::activateLine(void)
{
	SDL_Event event;
	bool run = true;

	t tmp;
	std::vector<t> centroids;
	int shRad=surface->w, shLine=0;

	// get centroids for each curve
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		tmp.xCenter = (*allLines[lineIterator].xPoints[0] + *allLines[lineIterator].xPoints[1] + *allLines[lineIterator].xPoints[2] + *allLines[lineIterator].xPoints[3]) / 4;
		tmp.yCenter = (*allLines[lineIterator].yPoints[0] + *allLines[lineIterator].yPoints[1] + *allLines[lineIterator].yPoints[2] + *allLines[lineIterator].yPoints[3]) / 4;
		tmp.lineIndex = lineIterator;
		centroids.push_back(tmp);
	}
	drawLines( grayedColor );
	while( run )
	{
		while( SDL_PollEvent(&event) )		// get events
		{
			switch( event.type )
			{
				case SDL_MOUSEMOTION:
// shRad=surface->w;
					shRad = dist( centroids[0].xCenter, centroids[0].yCenter, event.motion.x, event.motion.y );
					shLine = centroids[0].lineIndex;
					// scan each centroid - get distance from it to mouse
					for( int it = 0; it < centroids.size(); it++ )
					{
						if( dist( centroids[it].xCenter, centroids[it].yCenter, event.motion.x, event.motion.y ) < shRad )
						{
							shRad = dist( centroids[it].xCenter, centroids[it].yCenter, event.motion.x, event.motion.y );	// yech, change this
							shLine = centroids[it].lineIndex;
						}
					}
					doLockSurface = false;
					SDL_LockSurface( surface );
							drawLines( grayedColor );
							drawLine( allLines[shLine] );
					SDL_UnlockSurface( surface );
					SDL_Flip( surface );
					doLockSurface = true;
					break;
				case SDL_MOUSEBUTTONUP:
					if( event.button.button == SDL_BUTTON_RIGHT )
						return;
					break;
				default:
					break;
			}
		}
	}
}
/* Please! Work on this at some point! */
void Bezier::activateLine2(void)
{
	SDL_Event event;
	bool run = true;

	v tmp;
	std::vector<v> transforms;
	int shRad=surface->w, shLine=0;
	int mxRel, myRel;
	
	// dx1 is TL, dy1 is TR, dx2 is BL, dy2 is BR
	int dx1 = (*allLines[lineIterator].xPoints[1] - *allLines[lineIterator].xPoints[0]), dy1 = (*allLines[lineIterator].yPoints[1] - *allLines[lineIterator].yPoints[0]);
	int dx2 = (*allLines[lineIterator].xPoints[2] - *allLines[lineIterator].xPoints[0]), dy1 = (*allLines[lineIterator].yPoints[2] - *allLines[lineIterator].yPoints[0]);

	// transform each fourth point relative to others (unit vectors)
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		tmp.xOrigin = *allLines[lineIterator].xPoints[0];
		tmp.yOrigin = *allLines[lineIterator].yPoints[0];
		tmp.xRel = dx1 * *allLines[lineIterator].xPoints[3] + dy1 * *allLines[lineIterator].yPoints[3];
		tmp.yRel = dx2 * *allLines[lineIterator].xPoints[3] + dy2 * *allLines[lineIterator].yPoints[3];
		
		tmp.lineIndex = lineIterator;
		transforms.push_back(tmp);
	}
	drawLines( grayedColor );
	while( run )
	{
		while( SDL_PollEvent(&event) )		// get events
		{
			switch( event.type )
			{
				case SDL_MOUSEMOTION:
// shRad=surface->w;
					// scan each centroid - get distance from it to mouse
					// for( int it = 0; it < transforms.size(); it++ )
					// {
						// mxRel = dx1 * event.motion.x + dy1 * event.motion.y;
						// myRel = dx2 * event.motion.x + dy2 * event.motion.y;
						// if( dist( transforms[it].xCenter, transforms[it].yCenter, event.motion.x, event.motion.y ) < shRad )
						// {
							// shRad = dist( transforms[it].xCenter, transforms[it].yCenter, event.motion.x, event.motion.y );	// yech, change this
							// shLine = transforms[it].lineIndex;
						// }
					// }
					
					// transform mouse coordinates for each transforms curve point blah blah blah
					
					
					doLockSurface = false;
					SDL_LockSurface( surface );
							drawLines( grayedColor );
							drawLine( allLines[shLine] );
					SDL_UnlockSurface( surface );
					SDL_Flip( surface );
					doLockSurface = true;
					break;
				case SDL_MOUSEBUTTONUP:
					if( event.button.button == SDL_BUTTON_RIGHT )
						return;
					break;
				default:
					break;
			}
		}
	}
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
		// for( int i = 0; i < 4; i++ )
		// {
			// filledCircleRGBA( picking, *allLines[lineIterator].xPoints[i], *allLines[lineIterator].yPoints[i], 7, 255,255,lineIterator,255 );
			/* Draw control point circles */
			// circleRGBA( surface, *allLines[lineIterator].xPoints[i], *allLines[lineIterator].yPoints[i], 5, 255,255,255,255 );
			// circleColor( surface, *allLines[lineIterator].xPoints[i], *allLines[lineIterator].yPoints[i], 5, color - 0x8F );
		// }

		// lineRGBA( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], 128,128,128,100 );
		// lineRGBA( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], 128,128,128,100 );
		// lineRGBA( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[3], *allLines[lineIterator].yPoints[3], 128,128,128,100 );
		// lineRGBA( surface, *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], 128,128,128,100 );
		// lineRGBA( surface, *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], *allLines[lineIterator].xPoints[3], *allLines[lineIterator].yPoints[3], 128,128,128,100 );
		// lineRGBA( surface, *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], *allLines[lineIterator].xPoints[3], *allLines[lineIterator].yPoints[3], 128,128,128,100 );
		lineColor( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], grayedColor );
		lineColor( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], grayedColor );
		lineColor( surface, *allLines[lineIterator].xPoints[0], *allLines[lineIterator].yPoints[0], *allLines[lineIterator].xPoints[3], *allLines[lineIterator].yPoints[3], grayedColor );
		lineColor( surface, *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], grayedColor );
		lineColor( surface, *allLines[lineIterator].xPoints[1], *allLines[lineIterator].yPoints[1], *allLines[lineIterator].xPoints[3], *allLines[lineIterator].yPoints[3], grayedColor );
		lineColor( surface, *allLines[lineIterator].xPoints[2], *allLines[lineIterator].yPoints[2], *allLines[lineIterator].xPoints[3], *allLines[lineIterator].yPoints[3], grayedColor );

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
		if( active )	// dragging a point - highlight that point
			circleColor( surface, *allLines[activeLine].xPoints[allLines[activeLine].activePoint], *allLines[activeLine].yPoints[allLines[activeLine].activePoint], circleRadius, highlightColor );
	}
    if( doLockSurface )
    {
        SDL_UnlockSurface( surface );
        SDL_Flip( surface );
    }
}
/* TODO: Implement recursive subdivision to smooth out line sufficiently (or do a hack job and increase the number of points plotted) */
void Bezier::drawLine(bLine bl)
{
	int xNew, yNew, xOld, yOld;
	double step = 1.0/curvePoints;

	if( doLockSurface )
		SDL_LockSurface( surface );
	// SDL_FillRect( surface, NULL, 0 );	// clear surface to black
	for( int lineIterator = 0; lineIterator < allLines.size(); lineIterator++ )
	{
		/* Draw control points on picking surface */
		for( int i = 0; i < 4; i++ )
		{
			/* Draw control point circles */
			circleColor( surface, *bl.xPoints[i], *bl.yPoints[i], circleRadius, 0xFFFFFF3F );
		}

		lineColor( surface, *bl.xPoints[0], *bl.yPoints[0], *bl.xPoints[1], *bl.yPoints[1], grayedColor );
		lineColor( surface, *bl.xPoints[0], *bl.yPoints[0], *bl.xPoints[2], *bl.yPoints[2], grayedColor );
		lineColor( surface, *bl.xPoints[0], *bl.yPoints[0], *bl.xPoints[3], *bl.yPoints[3], grayedColor );
		lineColor( surface, *bl.xPoints[1], *bl.yPoints[1], *bl.xPoints[2], *bl.yPoints[2], grayedColor );
		lineColor( surface, *bl.xPoints[1], *bl.yPoints[1], *bl.xPoints[3], *bl.yPoints[3], grayedColor );
		lineColor( surface, *bl.xPoints[2], *bl.yPoints[2], *bl.xPoints[3], *bl.yPoints[3], grayedColor );

		/* Draw Bezier curve for current line */
		xOld = *bl.xPoints[0];
		yOld = *bl.yPoints[0];
		for( double t = 0+step, points = 0; points < curvePoints; t += step, points++ )
		{
			xNew = t*t*t*( *bl.xPoints[3] - *bl.xPoints[0] + 3*( *bl.xPoints[1] - *bl.xPoints[2] ) ) + 3*t*t*( *bl.xPoints[0] - 2**bl.xPoints[1] + *bl.xPoints[2] ) - 3*t*( *bl.xPoints[0] - *bl.xPoints[1] ) + ( *bl.xPoints[0] );
			yNew = t*t*t*( *bl.yPoints[3] - *bl.yPoints[0] + 3*( *bl.yPoints[1] - *bl.yPoints[2] ) ) + 3*t*t*( *bl.yPoints[0] - 2**bl.yPoints[1] + *bl.yPoints[2] ) - 3*t*( *bl.yPoints[0] - *bl.yPoints[1] ) + ( *bl.yPoints[0] );

			// circleRGBA( surface, xNew, yNew, 5, r,g,b,a/3 );
			// lineColor( surface, xNew, yNew, xOld, yOld, 0xFFFFFFFF );
			lineColor( surface, xNew, yNew, xOld, yOld, defaultColor );
			// circleRGBA( surface, xNew, yNew, 3, 255,255,255,255 );

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
inline double Bezier::dist(int x0, int y0, int x1, int y1)
{
	return (sqrt((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0)));
}
inline double Bezier::distLine(int x, int y, int lineIndex, int pointIndex)
{
	return sqrt( (x-*allLines[lineIndex].xPoints[pointIndex])*(x-*allLines[lineIndex].xPoints[pointIndex]) + (y-*allLines[lineIndex].yPoints[pointIndex])*(y-*allLines[lineIndex].yPoints[pointIndex]) );
}
/* Lock the surface before calling this function */
inline void Bezier::drawCircles(bLine &bl, Uint32 color)
{
	for( int i = 0; i < 4; i++ )
	{
		circleColor( surface, *bl.xPoints[i], *bl.yPoints[i], circleRadius, color );
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

*/
