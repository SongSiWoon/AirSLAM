#ifndef AIR_SLAM_TIMER_H_
#define AIR_SLAM_TIMER_H_

#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

class Timer {
public:
	using Clock = std::chrono::high_resolution_clock;
	using TimePoint = std::chrono::time_point<Clock>;
	using LogCallback = std::function<void(const std::string&)>;

	Timer();
	~Timer();

	void Start(const std::string& name);
	void Stop(const std::string& name);
	void SetLogCallback(LogCallback callback);

	double GetDuration(const std::string& name) const;
	void PrintLastFrameStats() const;
	void SaveToFile(const std::string& file_path);
	void NextFrame();

private:
	LogCallback log_callback_;
	std::map<std::string, TimePoint> start_times_;
	std::map<std::string, double> frame_durations_ms_;
	std::vector<std::map<std::string, double>> all_timings_;
	std::vector<std::string> ordered_keys_;
};

#endif // AIR_SLAM_TIMER_H_


