#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include "plnet.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    // Usage: ./plnet_inference <config_path> <input_dir> <output_dir>
    // config_path: Directory containing PLNet model files
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

    // Load PLNet configuration
    PLNetConfig plnet_config;
    plnet_config.plnet_s0_onnx = config_path + "/plnet_s0.onnx";
    plnet_config.plnet_s1_onnx = config_path + "/plnet_s1.onnx";
    plnet_config.plnet_s0_engine = config_path + "/plnet_s0.engine";
    plnet_config.plnet_s1_engine = config_path + "/plnet_s1.engine";
    plnet_config.keypoint_threshold = 0.004;
    plnet_config.line_threshold = 0.75;
    plnet_config.line_length_threshold = 50.0;
    plnet_config.remove_borders = 4;
    plnet_config.max_keypoints = 400;

    // Initialize PLNet
    PLNet plnet(plnet_config);
    if (!plnet.build()) {
        std::cerr << "Failed to build PLNet. Please check if ONNX files exist in: " << config_path << std::endl;
        return -1;
    }

    // Get list of image files
    std::vector<std::string> image_files;
    for (const auto& entry : fs::directory_iterator(input_dir)) {
        if (entry.path().extension() == ".png" || 
            entry.path().extension() == ".jpg" || 
            entry.path().extension() == ".jpeg") {
            image_files.push_back(entry.path().string());
        }
    }

    if (image_files.empty()) {
        std::cerr << "No valid image files found in: " << input_dir << std::endl;
        return -1;
    }

    // Process first 50 images only
    if (image_files.size() > 50) {
        image_files.resize(50);
        std::cout << "Processing first 50 images out of " << image_files.size() << " total images" << std::endl;
    }

    std::vector<double> inference_times;
    std::vector<int> keypoint_counts;
    std::vector<int> line_counts;

    for (const auto& image_path : image_files) {
        // Load image
        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (image.empty()) {
            std::cerr << "Failed to load image: " << image_path << " (invalid format or file not found)" << std::endl;
            continue;
        }

        // Initialize result variables
        Eigen::Matrix<float, 259, Eigen::Dynamic> features;
        std::vector<Eigen::Vector4d> lines;
        Eigen::Matrix<float, 259, Eigen::Dynamic> junctions;
        features.resize(0, 0);
        lines.clear();
        junctions.resize(0, 0);

        // Start inference time measurement
        auto start = std::chrono::high_resolution_clock::now();

        // Run inference
        bool success = plnet.infer(image, features, lines, junctions, true);

        // End inference time measurement
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        inference_times.push_back(elapsed);
        keypoint_counts.push_back(features.cols());
        line_counts.push_back(lines.size());

        std::cout << fs::path(image_path).filename().string() 
                  << " (size: " << image.cols << "x" << image.rows << ")"
                  << ": inference time = " << elapsed << " s"
                  << ", keypoints = " << features.cols()
                  << ", lines = " << lines.size() << std::endl;

        if (!success) {
            std::cerr << "Inference failed for image: " << image_path << std::endl;
            continue;
        }

        // Visualize results
        cv::Mat vis = image.clone();
        cv::cvtColor(vis, vis, cv::COLOR_GRAY2BGR);

        // Draw keypoints
        for (int i = 0; i < features.cols(); ++i) {
            int x = static_cast<int>(features(1, i));
            int y = static_cast<int>(features(2, i));
            float score = features(0, i);
            cv::Scalar color = cv::Scalar(0, 255 * score, 0);  // Color varies with score
            cv::circle(vis, cv::Point(x, y), 3, color, -1);
        }

        // Draw line segments
        for (const auto& line : lines) {
            cv::Point pt1(static_cast<int>(line[0]), static_cast<int>(line[1]));
            cv::Point pt2(static_cast<int>(line[2]), static_cast<int>(line[3]));
            cv::line(vis, pt1, pt2, cv::Scalar(0, 0, 255), 2);
        }

        // Save results
        std::string output_path = output_dir + "/result_" + fs::path(image_path).filename().string();
        if (!cv::imwrite(output_path, vis)) {
            std::cerr << "Failed to save visualization: " << output_path << std::endl;
        }
    }

    // Calculate statistics
    double avg_time = 0.0;
    double avg_keypoints = 0.0;
    double avg_lines = 0.0;
    
    for (size_t i = 0; i < inference_times.size(); ++i) {
        avg_time += inference_times[i];
        avg_keypoints += keypoint_counts[i];
        avg_lines += line_counts[i];
    }
    
    avg_time /= inference_times.size();
    avg_keypoints /= keypoint_counts.size();
    avg_lines /= line_counts.size();

    std::cout << "\nStatistics:" << std::endl;
    std::cout << "Processed " << inference_times.size() << " images" << std::endl;
    std::cout << "Average inference time: " << avg_time << " s" << std::endl;
    std::cout << "Average keypoints per image: " << avg_keypoints << std::endl;
    std::cout << "Average lines per image: " << avg_lines << std::endl;
    std::cout << "Average inference speed: " << (1.0 / avg_time) << " Hz" << std::endl;


    return 0;
} 