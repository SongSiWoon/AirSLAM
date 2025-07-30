#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <random>
#include "super_point.h"
#include "light_glue.h"
#include "point_matcher.h"
#include "read_configs.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    // Usage: ./light_glue_inference <config_path> <input_dir> <output_dir>
    // config_path: Directory containing model files
    // input_dir: Directory containing input images  
    // output_dir: Directory to save results
    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <config_path> <input_dir> <output_dir>" << std::endl;
        return -1;
    }

    std::string config_path = argv[1];
    std::string input_dir = argv[2];
    std::string output_dir = argv[3];

    // Create output directory
    if (!fs::exists(output_dir)) {
        try {
            fs::create_directories(output_dir);
            std::cout << "Created output directory: " << output_dir << std::endl;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Failed to create output directory: " << e.what() << std::endl;
            return -1;
        }
    } else {
        std::cout << "Output directory already exists: " << output_dir << std::endl;
    }

    // Configure SuperPoint
    SuperPointConfig superpoint_config;
    superpoint_config.max_keypoints = 400;
    superpoint_config.keypoint_threshold = 0.004;
    superpoint_config.remove_borders = 4;
    superpoint_config.dla_core = -1;
    superpoint_config.input_tensor_names.push_back("input");
    superpoint_config.output_tensor_names.push_back("scores");
    superpoint_config.output_tensor_names.push_back("descriptors");
    superpoint_config.onnx_file = config_path + "/superpoint_v1_sim_int32.onnx";
    superpoint_config.engine_file = config_path + "/superpoint_v1_sim_int32.engine";

    // Configure LightGlue
    PointMatcherConfig lightglue_config;
    lightglue_config.matcher = 0; // 0 for lightglue
    lightglue_config.image_width = 640;
    lightglue_config.image_height = 480;
    lightglue_config.onnx_file = config_path + "/superpoint_lightglue.onnx";
    lightglue_config.engine_file = config_path + "/superpoint_lightglue.engine";

    // Initialize SuperPoint
    SuperPoint superpoint(superpoint_config);
    if (!superpoint.build()) {
        std::cerr << "Failed to build SuperPoint. Please check if ONNX files exist in: " << config_path << std::endl;
        return -1;
    }

    // Initialize PointMatcher (includes LightGlue)
    PointMatcher point_matcher(lightglue_config);

    // Get list of image files
    std::vector<std::string> image_files;
    for (const auto& entry : fs::directory_iterator(input_dir)) {
        if (entry.path().extension() == ".png" || 
            entry.path().extension() == ".jpg" || 
            entry.path().extension() == ".jpeg") {
            image_files.push_back(entry.path().string());
        }
    }

    if (image_files.size() < 2) {
        std::cerr << "Need at least 2 images for matching. Found: " << image_files.size() << std::endl;
        return -1;
    }

    // Sort image files for consistent ordering
    std::sort(image_files.begin(), image_files.end());

    // Process pairs of images (limit to first 25 pairs)
    int max_pairs = std::min(25, (int)image_files.size() - 1);
    
    std::vector<double> inference_times;
    std::vector<int> match_counts;
    std::vector<int> keypoint_counts_0;
    std::vector<int> keypoint_counts_1;

    std::random_device rd;
    std::mt19937 gen(rd());

    for (int i = 0; i < max_pairs; ++i) {
        // Load two consecutive images
        cv::Mat image0 = cv::imread(image_files[i], cv::IMREAD_GRAYSCALE);
        cv::Mat image1 = cv::imread(image_files[i + 1], cv::IMREAD_GRAYSCALE);
        
        if (image0.empty() || image1.empty()) {
            std::cerr << "Failed to load images: " << image_files[i] << " or " << image_files[i + 1] << std::endl;
            continue;
        }

        // Resize images to match config dimensions
        cv::resize(image0, image0, cv::Size(lightglue_config.image_width, lightglue_config.image_height));
        cv::resize(image1, image1, cv::Size(lightglue_config.image_width, lightglue_config.image_height));

        // Extract SuperPoint features for both images
        Eigen::Matrix<float, 259, Eigen::Dynamic> features0, features1;
        
        auto start_feature = std::chrono::high_resolution_clock::now();
        
        bool success0 = superpoint.infer(image0, features0);
        bool success1 = superpoint.infer(image1, features1);
        
        if (!success0 || !success1) {
            std::cerr << "Feature extraction failed for image pair " << i << std::endl;
            continue;
        }

        // Start matching time measurement
        auto start_match = std::chrono::high_resolution_clock::now();

        // Perform matching using LightGlue
        std::vector<cv::DMatch> matches;
        int num_matches = point_matcher.MatchingPoints(features0, features1, matches, false);

        // End timing
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start_feature).count();
        
        inference_times.push_back(elapsed);
        match_counts.push_back(num_matches);
        keypoint_counts_0.push_back(features0.cols());
        keypoint_counts_1.push_back(features1.cols());

        std::cout << "Pair " << i << " (" 
                  << fs::path(image_files[i]).filename().string() << " -> " 
                  << fs::path(image_files[i + 1]).filename().string() << ")"
                  << ": total time = " << elapsed << " s"
                  << ", keypoints = " << features0.cols() << "/" << features1.cols()
                  << ", matches = " << num_matches << std::endl;

        // Visualize matches
        cv::Mat vis;
        cv::hconcat(image0, image1, vis);
        cv::cvtColor(vis, vis, cv::COLOR_GRAY2BGR);

        // Draw keypoints
        for (int j = 0; j < features0.cols(); ++j) {
            int x = static_cast<int>(features0(1, j));
            int y = static_cast<int>(features0(2, j));
            float score = features0(0, j);
            cv::Scalar color = cv::Scalar(0, 255 * score, 0);
            cv::circle(vis, cv::Point(x, y), 2, color, -1);
        }

        for (int j = 0; j < features1.cols(); ++j) {
            int x = static_cast<int>(features1(1, j)) + lightglue_config.image_width;
            int y = static_cast<int>(features1(2, j));
            float score = features1(0, j);
            cv::Scalar color = cv::Scalar(0, 255 * score, 0);
            cv::circle(vis, cv::Point(x, y), 2, color, -1);
        }

        // Draw matches with different colors
        std::uniform_int_distribution<int> color_dist(0, 255);
        for (const auto& match : matches) {
            if (match.queryIdx < features0.cols() && match.trainIdx < features1.cols()) {
                cv::Point pt1(static_cast<int>(features0(1, match.queryIdx)), 
                             static_cast<int>(features0(2, match.queryIdx)));
                cv::Point pt2(static_cast<int>(features1(1, match.trainIdx)) + lightglue_config.image_width, 
                             static_cast<int>(features1(2, match.trainIdx)));
                
                // Generate random color for each match
                cv::Scalar line_color(color_dist(gen), color_dist(gen), color_dist(gen));
                cv::line(vis, pt1, pt2, line_color, 1);
                
                // Draw match points with circles
                cv::circle(vis, pt1, 3, cv::Scalar(0, 0, 255), 2);
                cv::circle(vis, pt2, 3, cv::Scalar(0, 0, 255), 2);
            }
        }

        // Add text overlay with statistics
        std::string info_text = "Keypoints: " + std::to_string(features0.cols()) + "/" + std::to_string(features1.cols()) + 
                               ", Matches: " + std::to_string(num_matches) + 
                               ", Time: " + std::to_string(elapsed).substr(0, 5) + "s";
        cv::putText(vis, info_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        // Save result
        std::string output_path = output_dir + "/lightglue_match_" + std::to_string(i) + "_" + 
                                 fs::path(image_files[i]).stem().string() + "_to_" + 
                                 fs::path(image_files[i + 1]).stem().string() + ".png";
        if (!cv::imwrite(output_path, vis)) {
            std::cerr << "Failed to save visualization: " << output_path << std::endl;
        }
    }

    // Calculate statistics
    if (!inference_times.empty()) {
        double avg_time = 0.0;
        double avg_matches = 0.0;
        double avg_keypoints_0 = 0.0;
        double avg_keypoints_1 = 0.0;
        
        for (size_t i = 0; i < inference_times.size(); ++i) {
            avg_time += inference_times[i];
            avg_matches += match_counts[i];
            avg_keypoints_0 += keypoint_counts_0[i];
            avg_keypoints_1 += keypoint_counts_1[i];
        }
        
        avg_time /= inference_times.size();
        avg_matches /= match_counts.size();
        avg_keypoints_0 /= keypoint_counts_0.size();
        avg_keypoints_1 /= keypoint_counts_1.size();

        std::cout << "\n=== LightGlue Matching Statistics ===" << std::endl;
        std::cout << "Processed " << inference_times.size() << " image pairs" << std::endl;
        std::cout << "Average total time per pair: " << avg_time << " s" << std::endl;
        std::cout << "Average keypoints (img1/img2): " << avg_keypoints_0 << "/" << avg_keypoints_1 << std::endl;
        std::cout << "Average matches per pair: " << avg_matches << std::endl;
        std::cout << "Average processing speed: " << (1.0 / avg_time) << " Hz" << std::endl;

        // Find best and worst matching pairs
        auto max_matches_it = std::max_element(match_counts.begin(), match_counts.end());
        auto min_matches_it = std::min_element(match_counts.begin(), match_counts.end());
        
        if (max_matches_it != match_counts.end() && min_matches_it != match_counts.end()) {
            std::cout << "Best matching pair: " << *max_matches_it << " matches" << std::endl;
            std::cout << "Worst matching pair: " << *min_matches_it << " matches" << std::endl;
        }
    }

    return 0;
} 