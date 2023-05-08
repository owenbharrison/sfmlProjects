enum _cell_type_ {

};

struct flipFluid {
	// fluid
	int fNumX, fNumY, fNumCells;
	float density, scl, invScl;
	float* u, * v, * du, * dv;
	float* prevU, * prevV;
	float* p, * s;
	int* cellType;

	//particles
	int numParticles, maxParticles;
	float* particlePos, * particleVel, * particleDensity;
	float particleRestDensity;

	float particleRadius, pInvSpacing;
	int pNumX, pNumY, pNumCells;

	int* numCellParticles, * firstCellParticle, * cellParticleIds;

	flipFluid() {
		fNumX = fNumY = fNumCells = 0;
		density = scl = invScl = 0;
		u = v = du = dv = nullptr;
		prevU = prevV = nullptr;
		p = s = nullptr;
		cellType = nullptr;
		numParticles = maxParticles = 0;
		particlePos = particleVel = particleDensity = nullptr;
		particleRadius = pInvSpacing = 0;
		pNumX = pNumY = pNumCells = 0;
		numCellParticles = firstCellParticle = cellParticleIds = nullptr;
	}

	flipFluid(float density_, int width_, int height_, float spacing_, float particleRadius_, int maxParticles_) : density(density_), fNumX(width_), h(h_),
};