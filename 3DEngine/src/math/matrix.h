#pragma once

#include <cstring>

typedef unsigned int uint;
namespace displib {
	// rectangle matrix template
#define RCMT template <uint M, uint N>
// rectangle matrix
#define RCM matrix<M, N>

	RCMT struct matrix {
		static_assert(M > 0 && N > 0, "matrix dimension must be nonzero.");
		float v[M][N] = { 0.f };

		void operator=(const matrix& in) {
			memcpy(this->v, in.v, sizeof(float) * M * N);
		}

		bool operator==(const matrix& in) {
			for (uint i = 0; i < M; i++) {
				for (uint j = 0; j < N; j++) {
					if (this->v[i][j] != in.v[i][j]) {
						return false;
					}
				}
			}
			return true;
		}

		matrix operator+(const matrix& in) {
			matrix out;
			for (uint i = 0; i < M; i++) {
				for (uint j = 0; j < N; j++) {
					out.v[i][j] = this->v[i][j] + in.v[i][j];
				}
			}
			return out;
		}

		matrix operator*(const float& in) {
			matrix out;
			for (uint i = 0; i < M; i++) {
				for (uint j = 0; j < N; j++) {
					out.v[i][j] = this->v[i][j] * in;
				}
			}
			return out;
		}
	};

	// by definition disallows incompatible dimensions
	template <uint M, uint N, uint P>
	matrix<M, P> operator*(const RCM& a, const matrix<N, P>& b) {
		matrix<M, P> ab;
		for (uint i = 0; i < M; i++) {
			for (uint j = 0; j < P; j++) {
				float sum = 0.f;
				for (uint r = 0; r < N; r++) {
					sum += a.v[i][r] * b.v[r][j];
				}
				ab.v[i][j] = sum;
			}
		}
		return ab;
	}

	// flip about y=x axis
	RCMT matrix<N, M> transpose(const RCM& in) {
		matrix<N, M> out;
		for (uint i=0; i<M; i++) {
			for (uint j=0; j<N; j++) {
				out.v[j][i]=in.v[i][j];
			}
		}
		return out;
	}

#undef RCMT
#undef RCM
}