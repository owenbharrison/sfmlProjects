struct cmplx {
	float re, im;

	cmplx();

	cmplx(float re, float im);

	cmplx operator+(cmplx c);
	cmplx operator*(float f);
	cmplx operator*(cmplx c);

	static cmplx exponent(cmplx f);
};