/*
The MIT License (MIT)

Copyright (c) 2015 Lachlan Tychsen-Smith (lachlan.ts@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
Based on: 

		log(x) = log((1+m) * (2^n))
		log(x) = n * log(2) + log(1 + m)
		log(1+m) = Poly(1+m)
		
		where Poly(x) is the Minimax approximation of log(x) over the 
		range [1, 2]

Test func : logf(x)
Test Range: 1 < x < 10000
Peak Error:	~0.000601%
RMS  Error: ~0.000005%
*/

#include "math.h"
#include "math_neon.h"

const float __logf_rng =  0.693147180f;

const float __logf_lut[8] = {
	-2.295614848256274, 	//p0
	-2.470711633419806, 	//p4
	-5.686926051100417, 	//p2
	-0.165253547131978, 	//p6
	+5.175912446351073, 	//p1
	+0.844006986174912, 	//p5
	+4.584458825456749, 	//p3
	+0.014127821926000		//p7
};

float logf_c(float x)
{
	float a, b, c, d, xx;
	int m;
	
	union {
		float   f;
		int 	i;
	} r;
	
	//extract exponent
	r.f = x;
	m = (r.i >> 23);
	m = m - 127;
	r.i = r.i - (m << 23);
		
	//Taylor Polynomial (Estrins)
	xx = r.f * r.f;
	a = (__logf_lut[4] * r.f) + (__logf_lut[0]);
	b = (__logf_lut[6] * r.f) + (__logf_lut[2]);
	c = (__logf_lut[5] * r.f) + (__logf_lut[1]);
	d = (__logf_lut[7] * r.f) + (__logf_lut[3]);
	a = a + b * xx;
	c = c + d * xx;
	xx = xx * xx;
	r.f = a + c * xx;

	//add exponent
	r.f = r.f + ((float) m) * __logf_rng;

	return r.f;
}

float logf_neon_hfp(float x)
{
#ifdef __MATH_NEON
	asm volatile (
	
	"vdup.f32		d0, d0[0]				\n\t"	//d0 = {x,x};
	
	//extract exponent
	"vmov.i32		d2, #127				\n\t"	//d2 = 127;
	"vshr.u32		d6, d0, #23				\n\t"	//d6 = d0 >> 23;
	"vsub.i32		d6, d6, d2				\n\t"	//d6 = d6 - d2;
	"vshl.u32		d1, d6, #23				\n\t"	//d1 = d6 << 23;
	"vsub.i32		d0, d0, d1				\n\t"	//d0 = d0 + d1;

	//polynomial:
	"vmul.f32 		d1, d0, d0				\n\t"	//d1 = d0*d0 = {x^2, x^2}	
	"vld1.32 		{d2, d3, d4, d5}, [%1]	\n\t"	//q1 = {p0, p4, p2, p6}, q2 = {p1, p5, p3, p7} ;
	"vmla.f32 		q1, q2, d0[0]			\n\t"	//q1 = q1 + q2 * d0[0]		
	"vmla.f32 		d2, d3, d1[0]			\n\t"	//d2 = d2 + d3 * d1[0]		
	"vmul.f32 		d1, d1, d1				\n\t"	//d1 = d1 * d1 = {x^4, x^4}	
	"vmla.f32 		d2, d1, d2[1]			\n\t"	//d2 = d2 + d1 * d2[1]		

	//add exponent 	
	"vdup.32 		d7, %0					\n\t"	//d7 = {rng, rng}
	"vcvt.f32.s32 	d6, d6					\n\t"	//d6 = (float) d6
	"vmla.f32 		d2, d6, d7				\n\t"	//d2 = d2 + d6 * d7		

	"vmov.f32 		s0, s4					\n\t"	//s0 = s4

	:: "r"(__logf_rng), "r"(__logf_lut) 
    : "d0", "d1", "q1", "q2", "d6", "d7"
	);
#endif
}

float logf_neon_sfp(float x)
{
#ifdef __MATH_NEON
	asm volatile ("vmov.f32 s0, r0 		\n\t");
	logf_neon_hfp(x);
	asm volatile ("vmov.f32 r0, s0 		\n\t");
#else
	return logf_c(x);
#endif
};

