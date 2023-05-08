#include <vector>
#include "math/cmplx.hpp"

#define PI 3.1415927f

std::vector<cmplx> dft(std::vector<cmplx>& audio) {
	int N=audio.size();
	std::vector<cmplx> out;
	for (int k=0; k<N; k++) {
		cmplx sum;
		for (int n=0; n<N; n++) {
			cmplx t=cmplx(0.f, -2.f)*(PI*k*n/N);
			sum=sum+audio[n]*cmplx::exponent(t);
		}
		out.push_back(sum);
	}
	return out;
}

int main(){
	
	
	return 0;
}