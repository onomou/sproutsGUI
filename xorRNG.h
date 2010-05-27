
/* Marsaglia's XOR-Shift RNG - source : class PowerPoint */
/* This currently does not start with a random seed - very repeatable - yech */
inline unsigned long XORshiftRNG()
{
	static unsigned long x = 123456789, y = 362436069, z = 521288629, w = 88675123;
	unsigned long t = x^(x<<11);
	x = y;
	y = z;
	z = w;
	return (w = (w^(w>>19))^(t^(t>>8)));
}

/* Returns a random number in [0,n) */
inline int random( int n )
{
	// return XORshiftRNG() % n;	// use this once XOR-Shift RNG has a random start seed
	return rand() % n;
}
