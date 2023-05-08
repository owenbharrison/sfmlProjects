#include <cmath>
#include "cmplx.hpp"

cmplx::cmplx() : cmplx(0.f, 0.f) {}

cmplx::cmplx(float re, float im) : re(re), im(im) {}

cmplx cmplx::operator+(cmplx c) {
	return {this->re+c.re, this->im+c.im};
}

cmplx cmplx::operator*(float f) {
	return {this->re*f, this->re*f};
}
cmplx cmplx::operator*(cmplx c) {
	return {
		this->re*c.re-this->im*c.im,
		this->re*c.im+this->im*c.re
	};
}

cmplx cmplx::exponent(cmplx f) {
	float out=expf(f.re);
	return {cosf(f.im)*out, sinf(f.im)*out};
}