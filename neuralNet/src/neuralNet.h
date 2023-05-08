#include "layer.h"
#include "dataPoint.h"

#pragma once
struct neuralNet {
	int numLayers = 0;
	layer* layers;

	neuralNet() {
		numLayers = 0;
		layers = nullptr;
	}

	neuralNet(std::vector<int> layerSizes) : numLayers(layerSizes.size() - 1) {
		layers = new layer[numLayers];
		for (int i = 0; i < numLayers; i++) {
			layers[i] = layer(layerSizes[i], layerSizes[i + 1]);
		}
	}

	std::vector<float> calculateOutputs(std::vector<float>& inputs) {
		std::vector<float> outputs{ inputs };

		for (int i = 0; i < numLayers; i++) {
			layer& l = layers[i];
			outputs = l.calculateOutputs(outputs);
		}

		return outputs;
	}

	int classify(std::vector<float>& inputs) {
		std::vector<float> outputs = calculateOutputs(inputs);
		float maxValue = -INFINITY;
		int maxIndex = -1;
		for (int i = 0; i < outputs.size(); i++) {
			float& value = outputs[i];
			if (value > maxValue) {
				maxValue = value;

				maxIndex = i;
			}
		}
		return maxIndex;
	}

	//a.k.a loss
	float cost(dataPoint d) {
		std::vector<float> outputs = calculateOutputs(d.inputs);
		layer& outputLayer = layers[numLayers - 1];

		float cost = 0;
		for (int o = 0; o < outputs.size(); o++) {
			cost += outputLayer.nodeCost(outputs[o], d.expectedOutputs[o]);
		}

		return cost;
	}

	float cost(std::vector<dataPoint> data) {
		float totalCost = 0;
		for (const auto& d : data) {
			totalCost += cost(d);
		}
		return totalCost / data.size();
	}

	//run a single iter of gradient descent
	void learn(std::vector<dataPoint> data, float learnRate) {
		const float h = 0.0001;
		float originalCost = cost(data);

		for (int k = 0; k < numLayers; k++) {
			layer& l = layers[k];

			//calc cost gradient for current weights
			for (int i = 0; i < l.numIn; i++) {
				for (int o = 0; o < l.numOut; o++) {
					l.weights[l.ix(i, o)] += h;
					float deltaCost = cost(data) - originalCost;
					l.weights[l.ix(i, o)] -= h;
					l.costGradientW[l.ix(i, o)] = deltaCost / h;
				}
			}

			//calc cost gradient for current biases
			for (int b = 0; b < l.numOut; b++) {
				l.biases[b] += h;
				float deltaCost = cost(data) - originalCost;
				l.biases[b] -= h;
				l.costGradientB[b] = deltaCost / h;
			}
		}

		//apply all gradients
		for (int i = 0; i < numLayers; i++) {
			layers[i].applyGradients(learnRate);
		}
	}

	void updateAllGradients(dataPoint d) {
		calculateOutputs(d.inputs);

		layer& outputLayer = layers[numLayers - 1];
		std::vector<float> nodeValues = outputLayer.calculateOutputLayerNodeValues(d.expectedOutputs);
	}
};