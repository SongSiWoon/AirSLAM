#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include "plnet.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    // 사용법: ./plnet_inference <config_path> <input_dir> <output_dir>
    // config_path: PLNet 모델 파일들이 있는 디렉토리
    // input_dir: 처리할 이미지들이 있는 디렉토리
    // output_dir: 결과를 저장할 디렉토리
    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <config_path> <input_dir> <output_dir>" << std::endl;
        return -1;
    }

    std::string config_path = argv[1];
    std::string input_dir = argv[2];
    std::string output_dir = argv[3];

    // 출력 디렉토리 생성
    if (!fs::create_directories(output_dir)) {
        std::cerr << "Failed to create output directory: " << output_dir << std::endl;
        return -1;
    }

    // PLNet 설정 로드
    PLNetConfig plnet_config;
    plnet_config.plnet_s0_onnx = config_path + "/plnet_s0.onnx";
    plnet_config.plnet_s1_onnx = config_path + "/plnet_s1.onnx";
    plnet_config.plnet_s0_engine = config_path + "/plnet_s0.engine";
    plnet_config.plnet_s1_engine = config_path + "/plnet_s1.engine";
    plnet_config.keypoint_threshold = 0.015;
    plnet_config.line_threshold = 0.5;
    plnet_config.line_length_threshold = 3.0;
    plnet_config.remove_borders = 4;
    plnet_config.max_keypoints = 1000;

    // PLNet 초기화
    PLNet plnet(plnet_config);
    if (!plnet.build()) {
        std::cerr << "Failed to build PLNet. Please check if ONNX files exist in: " << config_path << std::endl;
        return -1;
    }

    // 이미지 파일 목록 가져오기
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

    // 첫 50개 이미지만 처리
    if (image_files.size() > 50) {
        image_files.resize(50);
        std::cout << "Processing first 50 images out of " << image_files.size() << " total images" << std::endl;
    }

    std::vector<double> inference_times;
    std::vector<int> keypoint_counts;
    std::vector<int> line_counts;

    for (const auto& image_path : image_files) {
        // 이미지 로드
        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (image.empty()) {
            std::cerr << "Failed to load image: " << image_path << " (invalid format or file not found)" << std::endl;
            continue;
        }

        // 결과 저장용 변수 초기화
        Eigen::Matrix<float, 259, Eigen::Dynamic> features;
        std::vector<Eigen::Vector4d> lines;
        Eigen::Matrix<float, 259, Eigen::Dynamic> junctions;
        features.resize(0, 0);
        lines.clear();
        junctions.resize(0, 0);

        // 추론 시간 측정 시작
        auto start = std::chrono::high_resolution_clock::now();

        // 추론 실행
        bool success = plnet.infer(image, features, lines, junctions, true);

        // 추론 시간 측정 종료
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

        // 결과 시각화
        cv::Mat vis = image.clone();
        cv::cvtColor(vis, vis, cv::COLOR_GRAY2BGR);

        // 키포인트 그리기
        for (int i = 0; i < features.cols(); ++i) {
            int x = static_cast<int>(features(1, i));
            int y = static_cast<int>(features(2, i));
            float score = features(0, i);
            cv::Scalar color = cv::Scalar(0, 255 * score, 0);  // 점수에 따라 색상 변화
            cv::circle(vis, cv::Point(x, y), 3, color, -1);
        }

        // 선분 그리기
        for (const auto& line : lines) {
            cv::Point pt1(static_cast<int>(line[0]), static_cast<int>(line[1]));
            cv::Point pt2(static_cast<int>(line[2]), static_cast<int>(line[3]));
            cv::line(vis, pt1, pt2, cv::Scalar(0, 0, 255), 2);
        }

        // 결과 저장
        std::string output_path = output_dir + "/result_" + fs::path(image_path).filename().string();
        if (!cv::imwrite(output_path, vis)) {
            std::cerr << "Failed to save visualization: " << output_path << std::endl;
        }
    }

    // 통계 계산
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

    return 0;
} 