
/*
 * This is a part of a working program to resample a scanline.
 * IN  is a pointer to the  input scanline having  INlen pixels.
 * OUT is a pointer to the output scanline having OUTlen pixels.
 * offset is the inter-pixel offset: 1 for rows, width for columns
 * filtertype specifies which filter to use for convolution:
 *	0) box; 1) triangle; 2) cubic convolution; 3) Lanczos3;
 *	4) 8-point windowed sinc function using a Hann window
 */

#define PI		3.1415926535897931160E0
#define SGN(A)		((A) > 0 ? 1 : ((A) < 0 ? -1 : 0 ))
#define FLOOR(A)	((int) (A))
#define CEILING(A)	((A)==FLOOR(A) ? FLOOR(A) : SGN(A)+FLOOR(A))
#define CLAMP(A,L,H)	((A)<=(L) ? (L) : (A)<=(H) ? (A) : (H))

resample1D(IN, OUT, INlen, OUTlen, filtertype, offset)
unsigned char *IN, *OUT;
int INlen, OUTlen, filtertype, offset;
{
	int i;
	int left, right;	/* kernel extent in input  */
	int pixel;		/* input pixel value	   */
	double u, x;		/* input (u) , output (x)  */
	double scale;		/* resampling scale factor */
	double (*filter)();	/* pointer to filter fct   */
	double fwidth;		/* filter width (support)  */
	double fscale;		/* filter amplitude scale  */
	double weight;		/* kernel weight	   */
	double acc;		/* convolution accumulator */

	scale = (double) OUTlen / INlen;

	switch(filtertype) {
	case 0: filter = boxFilter;	/* box filter (nearest neighbor)  */
		fwidth = .5;
		break;
	case 1:	filter = triFilter;	/* triangle filter (linear intrp) */
		fwidth = 1;
		break;
	case 2:	filter = cubicConv;	/* cubic convolution filter 	  */
		fwidth = 2;
		break;
	case 3:	filter = lanczos3;	/* Lanczos3 windowed sinc function */
		fwidth = 3;
		break;
	case 4:	filter = hann4;	/* Hann windowed sinc function */
		fwidth = 4;			/* 8-point kernel */
		break;
	}

	if(scale < 1.0) {		/* minification: h(x) -> h(x*scale)*scale */
		fwidth = fwidth / scale;		/* broaden  filter */
		fscale = scale;			/* lower amplitude */

		/* roundoff fwidth to int to avoid intensity modulation */
		if(filtertype == 0) {
			fwidth = CEILING(fwidth);
			fscale = 1.0 / (2*fwidth);
		}
	} else	fscale = 1.0;

	/* project each output pixel to input, center kernel, and convolve */
	for(x=0; x<OUTlen; x++) {
		/* map output x to input u: inverse mapping */
		u = x / scale;

		/* left and right extent of kernel centered at u */
		if(u - fwidth < 0) {
			left = FLOOR  (u - fwidth);
		else	left = CEILING(u - fwidth);
		right = FLOOR(u + fwidth);

		/* reset acc for collecting convolution products */
		acc = 0;

		/* weigh input pixels around u with kernel */
		for(i=left; i <= right; i++) {
			pixel  = IN[ CLAMP(i, 0, INlen-1)*offset];
			weight = (*filter)((u - i) * fscale);
			acc   += (pixel * weight);
		}

		/* assign weighted accumulator to OUT */
		OUT[x*offset] = acc * fscale;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * boxFilter:
 *
 * Box (nearest neighbor) filter.
 */
double
boxFilter(t)
double t;
{
	if((t > -.5) && (t <= .5)) return(1.0);
	return(0.0);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * triFilter:
 *
 * Triangle filter (used for linear interpolation).
 */
double
triFilter(t)
double t;
{
	if(t < 0) t = -t;
	if(t < 1.0) return(1.0 - t);
	return(0.0);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * cubicConv:
 *
 * Cubic convolution filter.
 */
double
cubicConv(t)
double t;
{
	double A, t2, t3;

	if(t < 0) t = -t;
	t2 = t  * t;
	t3 = t2 * t;

	A = -1.0;	/* user-specified free parameter */
	if(t < 1.0) return((A+2)*t3 - (A+3)*t2 + 1);
	if(t < 2.0) return(A*(t3 - 5*t2 + 8*t - 4));
	return(0.0);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * sinc:
 *
 * Sinc function.
 */
double
sinc(t)
double t;
{
	t *= PI;
	if(t != 0) return(sin(t) / t);
	return(1.0);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * lanczos3:
 *
 * Lanczos3 filter.
 */
double
lanczos3(t)
double t;
{
	if(t < 0) t = -t;
	if(t < 3.0) return(sinc(t) * sinc(t / 3.0));
	return(0.0);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * hann:
 *
 * Hann windowed sinc function. Assume N (width) = 4.
 */
double
hann4(t)
double t;
{
	int N = 4;	/* fixed filter width */

	if(t < 0) t = -t;
	if(t < N) return(sinc(t) * (.5 + .5*cos(PI*t / N)));
	return(0.0);
}
