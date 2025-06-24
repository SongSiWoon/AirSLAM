#include <stdlib.h>

#include "timer.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include "utils.h"

Timer::Timer() {
}

Timer::~Timer() {
}

void Timer::Start(const std::string& name) {
	start_times_[name] = Clock::now();
	if (std::find(ordered_keys_.begin(), ordered_keys_.end(), name) ==
		ordered_keys_.end()) {
		ordered_keys_.push_back(name);
	}
}

void Timer::Stop(const std::string& name) {
	auto end_time = Clock::now();
	auto start_it = start_times_.find(name);
	if (start_it != start_times_.end()) {
		double duration = std::chrono::duration<double, std::milli>(end_time - start_it->second).count();
		frame_durations_ms_[name] += duration;
	}
}

double Timer::GetDuration(const std::string& name) const {
	auto it = frame_durations_ms_.find(name);
	if (it != frame_durations_ms_.end()) {
		return it->second;
	}
	return 0.0;
}

void Timer::NextFrame() {
	if (!frame_durations_ms_.empty()) {
		all_timings_.push_back(frame_durations_ms_);
	}
	frame_durations_ms_.clear();
}

void Timer::PrintLastFrameStats() const {
	if (all_timings_.empty()) {
		std::cout << "No timing data recorded yet." << std::endl;
		return;
	}

	const auto& last_frame_durations = all_timings_.back();
	std::cout << "--- Last Frame Timings (ms) ---" << std::endl;
	for (const auto& key : ordered_keys_) {
		auto it = last_frame_durations.find(key);
		if (it != last_frame_durations.end()) {
			std::cout << std::left << std::setw(30) << key << ": " << std::fixed
						<< std::setprecision(3) << it->second << std::endl;
		}
	}
	std::cout << "---------------------------------" << std::endl;
}

void Timer::SaveToFile(const std::string& file_path) {
	if (all_timings_.empty()) {
		return;
	}

	std::string::size_type pos = file_path.find_last_of('/');
    if (pos != std::string::npos) {
        std::string dir_path = file_path.substr(0, pos);
        if (!dir_path.empty()) {
            MakeDir(dir_path);
        }
    }

	struct Stats {
		double average = 0.0;
		double min_val = 0.0;
		double max_val = 0.0;
		double median = 0.0;
		double std_dev = 0.0;
	};
	std::map<std::string, Stats> summary_stats;

	for (const auto& key : ordered_keys_) {
		std::vector<double> values;
		for (const auto& frame_data : all_timings_) {
			auto it = frame_data.find(key);
			if (it != frame_data.end() && it->second > 0) {
				values.push_back(it->second);
			}
		}

		Stats stats;
		if (!values.empty()) {
			double sum = std::accumulate(values.begin(), values.end(), 0.0);
			stats.average = sum / values.size();

			std::sort(values.begin(), values.end());
			stats.min_val = values.front();
			stats.max_val = values.back();
			stats.median = values[values.size() / 2];

			double variance_sum = 0.0;
			for(const double& val : values) {
				variance_sum += (val - stats.average) * (val - stats.average);
			}
			stats.std_dev = std::sqrt(variance_sum / values.size());
		}
		summary_stats[key] = stats;
	}


	std::ofstream file(file_path);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open file for writing: " << file_path
				  << std::endl;
		return;
	}

	file << "--- Summary Statistics (ms) ---\n";
	file << "Metric,Average,Min,Max,Median,StdDev\n";
	for (const auto& key : ordered_keys_) {
		const auto& stats = summary_stats.at(key);
		file << key << "," << stats.average << "," << stats.min_val << "," << stats.max_val << "," << stats.median << "," << stats.std_dev << std::endl;
	}

	file << "\n--- Raw Frame Timings (ms) ---\n";
	file << "Frame";
	for (const auto& key : ordered_keys_) {
		file << "," << key;
	}
	file << std::endl;

	for (size_t i = 0; i < all_timings_.size(); ++i) {
		file << i;
		const auto& frame_data = all_timings_[i];
		for (const auto& key : ordered_keys_) {
			auto it = frame_data.find(key);
			if (it != frame_data.end()) {
				file << "," << it->second;
			} else {
				file << ",0";
			}
		}
		file << std::endl;
	}

	file.close();
}
