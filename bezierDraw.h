#include <algorithm>
#include <vector>
#include <cmath>

static const Uint32 defaultColor = 0xFFFFFFFF;
static const int defaultWidth = 1;
/*
drawBezier( SDL_Surface *surface, int ax, int ay, int bx, int by, int cx, int cy, int dx, int dy, Uint32 color = defaultColor, int width = defaultWidth )
{
	int x, y, xOld = ax, yOld = ay;
	int curvePoints = 30;
	double step = 1.0/curvePoints;

	lineColor( surface, ax, ay, bx, by, color/3 );		//
	lineColor( surface, ax, ay, cx, cy, color/3 );		//
	lineColor( surface, ax, ay, dx, dy, color/3 );		// boundary
	lineColor( surface, bx, by, cx, cy, color/3 );		//  lines
	lineColor( surface, bx, by, dx, dy, color/3 );		//
	lineColor( surface, cx, cy, dx, dy, color/3 );		//

	for( double t = 0; t <= 1; t += step )
	{
		x = t*t*t*( dx - ax + 3*( bx - cx ) ) + 3*t*t*( ax - 2*bx + cx ) - 3*t*( ax - bx ) + ( ax );
		y = t*t*t*( dy - ay + 3*( by - cy ) ) + 3*t*t*( ay - 2*by + cy ) - 3*t*( ay - by ) + ( ay );
		lineColor( surface, x, y, xOld, yOld, color );
		xOld = x;
		yOld = y;
	}
}*/
