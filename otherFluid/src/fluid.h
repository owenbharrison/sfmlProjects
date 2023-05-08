#define OVERRELAXATION 1.9f

#define U_FIELD 0
#define V_FIELD 1
#define SMOKE_FIELD 2

#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))

struct fluid {
	float density;
	int w, h, sz;
	float scl;
	float* u, * v, * newU, * newV;
	bool* fluidity;//s
	float* pressure, * smoke;//p, m
	float* newSmoke;//newM

	fluid() {
		density=0;
		w=h=sz=0;
		scl=0;
		u=v=newU=newV=nullptr;
		pressure=smoke=newSmoke=nullptr;
		fluidity=nullptr;
	}

	fluid(float density_, int w_, int h_, float scl_) : density(density_), w(w_), h(h_), scl(scl_) {
		sz=w*h;

		u=new float[sz];
		v=new float[sz];
		newU=new float[sz];
		newV=new float[sz];

		fluidity=new bool[sz];
		pressure=new float[sz];
		smoke=new float[sz];
		for (int i=0; i<sz; i++) { smoke[i]=1; }
		newSmoke=new float[sz];
	}

	int ix(int i, int j) {
		return i+j*w;
	}

	void solveIncompressibility(int numIters, float dt) {
		float cp=density*scl/dt;

		for (int iter=0; iter<numIters; iter++) {
			for (int i=1; i<w-1; i++) {
				for (int j=1; j<h-1; j++) {

					if (!fluidity[ix(i, j)]) continue;

					bool sx0=fluidity[ix(i-1, j)];
					bool sx1=fluidity[ix(i+1, j)];
					bool sy0=fluidity[ix(i, j-1)];
					bool sy1=fluidity[ix(i, j+1)];
					int s=sx0+sx1+sy0+sy1;
					if (s==0) continue;

					float div=u[ix(i+1, j)]-u[ix(i, j)]+
						v[ix(i, j+1)]-v[ix(i, j)];

					float p=-div/s;
					p*=OVERRELAXATION;
					pressure[ix(i, j)]+=cp*p;

					u[ix(i, j)]-=sx0*p;
					u[ix(i+1, j)]+=sx1*p;
					v[ix(i, j)]-=sy0*p;
					v[ix(i, j+1)]+=sy1*p;
				}
			}
		}
	}

	void extrapolate() {
		for (int i=0; i<w; i++) {
			u[ix(i, 0)]=u[ix(i, 1)];
			u[ix(i, h-1)]=u[ix(i, h-2)];
		}
		for (int j=0; j<h; j++) {
			v[ix(0, j)]=v[ix(1, j)];
			v[ix(w-1, j)]=v[ix(w-2, j)];
		}
	}

	float sampleField(float x, float y, int field) {
		float scl1=1/scl;
		float scl2=.5f*scl;

		x=MAX(MIN(x, w*scl), scl);
		y=MAX(MIN(y, h*scl), scl);

		float dx=0, dy=0;

		float* f;

		switch (field) {
			case U_FIELD:
				f=u, dy=scl2; break;
			case V_FIELD:
				f=v, dx=scl2; break;
			case SMOKE_FIELD: default:
				f=smoke, dx=scl2, dy=scl2; break;
		}

		int x0=MIN((int)floor((x-dx)*scl1), w-1);
		float tx=((x-dx)-x0*scl)*scl1;
		int x1=MIN(x0+1, w-1);

		int y0=MIN((int)floor((y-dy)*scl1), h-1);
		float ty=((y-dy)-y0*scl)*scl1;
		int y1=MIN(y0+1, h-1);

		float sx=1-tx;
		float sy=1-ty;

		return sx*sy*f[ix(x0, y0)]+
			tx*sy*f[ix(x1, y0)]+
			tx*ty*f[ix(x1, y1)]+
			sx*ty*f[ix(x0, y1)];
	}

	float avgU(int i, int j) {
		return (u[ix(i, j-1)]+u[ix(i, j)]+
			u[ix(i+1, j-1)]+u[ix(i+1, j)])*.25f;
	}

	float avgV(int i, int j) {
		return (v[ix(i, j-1)]+v[ix(i, j)]+
			v[ix(i+1, j-1)]+v[ix(i+1, j)])*.25f;
	}

	void advectVel(float dt) {
		memcpy(newU, u, sz*sizeof(float));
		memcpy(newV, v, sz*sizeof(float));

		float scl2=.5f*scl;

		for (int i=1; i<w; i++) {
			for (int j=1; j<h; j++) {
				// u component
				if (fluidity[ix(i, j)]&&fluidity[ix(i-1, j)]&&j<h-1) {
					float x=i*scl;
					float y=j*scl+scl2;
					float u_=u[ix(i, j)];
					float v_=avgV(i, j);
					x-=dt*u_;
					y-=dt*v_;
					newU[ix(i, j)]=sampleField(x, y, U_FIELD);
				}
				// v component
				if (fluidity[ix(i, j)]&&fluidity[ix(i, j-1)]&&i<w-1) {
					float x=i*scl+scl2;
					float y=j*scl;
					float u_=avgU(i, j);
					float v_=v[ix(i, j)];
					x-=dt*u_;
					y-=dt*v_;
					newV[ix(i, j)]=sampleField(x, y, V_FIELD);
				}
			}
		}

		memcpy(u, newU, sz*sizeof(float));
		memcpy(v, newV, sz*sizeof(float));
	}

	void advectSmoke(float dt) {
		memcpy(newSmoke, smoke, sz*sizeof(float));

		float scl2=.5f*scl;

		for (int i=1; i<w-1; i++) {
			for (int j=1; j<h-1; j++) {

				if (fluidity[ix(i, j)]) {
					float u_=(u[ix(i, j)]+u[ix(i+1, j)])*.5f;
					float v_=(v[ix(i, j)]+v[ix(i, j+1)])*.5f;
					float x=i*scl+scl2-dt*u_;
					float y=j*scl+scl2-dt*v_;

					newSmoke[ix(i, j)]=sampleField(x, y, SMOKE_FIELD);
				}
			}
		}
		memcpy(smoke, newSmoke, sz*sizeof(float));
	}

	void simulate(float dt, int numIters) {
		for (int i=0; i<sz; i++) { pressure[i]=0; }
		solveIncompressibility(numIters, dt);

		extrapolate();
		advectVel(dt);
		advectSmoke(dt);
	}
};