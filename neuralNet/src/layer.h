#include <vector>
#include <cmath>

#define RANDOM (rand()/32767.f)

float activation(float weightedInput) {
	return 1 / (1 + expf(-weightedInput));
}

float activationDerivative(float weightedInput) {

}

#pragma once
struct layer {
	int numIn, numOut;
	float* weights;
	float* costGradientW;
	float* biases;
	float* costGradientB;

	layer() {
		numIn = numOut = 0;
		weights = costGradientW = nullptr;
		biases = costGradientB = nullptr;
	}

	layer(int numIn_, int numOut_) : numIn(numIn_), numOut(numOut_) {
		weights = new float[numIn * numOut] {0};
		costGradientW = new float[numIn * numOut] {0};
		biases = new float[numOut] {0};
		costGradientB = new float[numOut] {0};
		initRandomWeights();
	}

	int ix(int i, int j) const { return i + j * numIn; }

	void initRandomWeights() {
		for (int i = 0; i < numIn; i++) {
			for (int o = 0; o < numOut; o++) {
				float randVal = RANDOM * 2 - 1;
				weights[ix(i, o)] = randVal / sqrtf(numIn);
			}
		}
	}

	//update weights and biases based on cost gradients
	void applyGradients(float learnRate) {
		for (int o = 0; o < numOut; o++) {
			biases[o] -= costGradientB[o] * learnRate;
			for (int i = 0; i < numIn; i++) {
				weights[ix(i, o)] -= costGradientW[ix(i, o)] * learnRate;
			}
		}
	}

	std::vector<float> calculateOutputs(std::vector<float>& inputs) {
		std::vector<float> outputs;
		for (int o = 0; o < numOut; o++) {
			float weightedInput = biases[o];
			for (int i = 0; i < numIn; i++) {
				weightedInput += inputs[i] * weights[ix(i, o)];
			}
			outputs.push_back(activation(weightedInput));
		}
		return outputs;
	};

	float nodeCost(float outputActivation, float expectedOutput) {
		float error = outputActivation - expectedOutput;
		return error * error;
	}

	float nodeCostDerivative(float outputActivation, float expectedOutput) {
		return 2 * (outputActivation - expectedOutput);
	}
};