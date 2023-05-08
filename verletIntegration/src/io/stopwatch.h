#include <chrono>

#pragma once
class stopwatch {
public:
	std::chrono::steady_clock::time_point startTime, endTime;

	stopwatch() {}

	std::chrono::steady_clock::time_point getTime() {
		return std::chrono::high_resolution_clock::now();
	}

	void start() {
		this->startTime = this->getTime();
	}

	void stop() {
		this->endTime = this->getTime();
	}

	int getNanoseconds() {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
	}

	int getMicroseconds() {
		return std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
	}

	int getMilliseconds() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	}

	int getSeconds() {
		return std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
	}
};