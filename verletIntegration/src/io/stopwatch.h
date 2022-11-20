#include <chrono>
using namespace std::chrono;

#pragma once
class stopwatch {
	public:
	steady_clock::time_point startTime, endTime;

	stopwatch(){}

	steady_clock::time_point getTime() {
		return high_resolution_clock::now();
	}

	void start() {
		this->startTime=this->getTime();
	}

	void stop() {
		this->endTime=this->getTime();
	}

	int getNanoseconds() {
		return duration_cast<nanoseconds>(endTime-startTime).count();
	}

	int getMicroseconds() {
		return duration_cast<microseconds>(endTime-startTime).count();
	}

	int getMilliseconds() {
		return duration_cast<milliseconds>(endTime-startTime).count();
	}

	int getSeconds() {
		return duration_cast<seconds>(endTime-startTime).count();
	}
};