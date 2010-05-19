#include <vector>
class Bezier
{
	private:
		struct bLine
		{
			int *points[4];
			int activePoint;
		};
		vector<bLine> allLines;
		int activeLine;
	public:
		Bezier();
		void addLine(int,int,int,int,int,int,int,int);
};
Bezier::Bezier()
{
}
void Bezier::addLine(int ax, int ay, int bx, int by, int cx, int cy, int dx, int dy)
{
}
