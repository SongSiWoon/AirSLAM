#include <stdlib.h>

#include "timer.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include "utils.h"

Timer::Timer() {
	// Constructor
}

Timer::~Timer() {
	// Destructor
}

void Timer::Start(const std::string& name) {
	start_times_[name] = Clock::now();
	// if key is new, add to ordered_keys_
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

	// --- 1. Calculate Summary Statistics First ---
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
		double sum = 0.0;
		for (const auto& frame_data : all_timings_) {
			auto it = frame_data.find(key);
			if (it != frame_data.end()) {
				if(it->second > 0) {
					values.push_back(it->second);
				}
				sum += it->second;
			}
		}

		Stats stats;
		stats.average = all_timings_.empty() ? 0.0 : sum / all_timings_.size();

		if (!values.empty()) {
			std::sort(values.begin(), values.end());
			stats.min_val = values.front();
			stats.max_val = values.back();
			stats.median = values[values.size() / 2];

			double variance_sum = 0.0;
			double values_mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
			for(const double& val : values) {
				variance_sum += (val - values_mean) * (val - values_mean);
			}
			stats.std_dev = std::sqrt(variance_sum / values.size());
		}
		summary_stats[key] = stats;
	}


	// --- 2. Write to file ---
	std::ofstream file(file_path);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open file for writing: " << file_path
				  << std::endl;
		return;
	}

	// Write Summary Statistics
	file << "--- Summary Statistics (ms) ---\n";
	file << "Metric,Average,Min,Max,Median,StdDev\n";
	for (const auto& key : ordered_keys_) {
		const auto& stats = summary_stats.at(key);
		file << key << "," << stats.average << "," << stats.min_val << "," << stats.max_val << "," << stats.median << "," << stats.std_dev << std::endl;
	}

	// Write raw data
	file << "\n--- Raw Frame Timings (ms) ---\n";
	file << "Frame";
	for (const auto& key : ordered_keys_) {
		file << "," << key;
	}
	file << std::endl;

	// Data
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
