#ifndef VO_CORE_H_
#define VO_CORE_H_

#include <memory>
#include <opencv2/opencv.hpp>
#include <Eigen/Core>
#include <rclcpp/rclcpp.hpp>

#include "read_configs.h"
#include "map_builder.h"
#include "dataset.h"

class VOCore {
public:
    VOCore(const VisualOdometryConfigs& configs, 
           const rclcpp::Node::SharedPtr& node = nullptr);
    ~VOCore();

    // Dataset processing interface
    bool ProcessDataset(const std::string& dataroot);
    
    // Real-time processing interface
    void ProcessImage(const cv::Mat& left_image, const cv::Mat& right_image, 
                     const ImuDataList& imu_data, double timestamp);
    
    // Common interfaces
    void Stop();
    bool IsStopped();
    void SaveTrajectory(const std::string& file_path);
    void SaveMap(const std::string& map_root);

private:
    VisualOdometryConfigs configs_;
    rclcpp::Node::SharedPtr node_;  // Optional node for logging
    std::shared_ptr<MapBuilder> map_builder_;
    std::shared_ptr<Dataset> dataset_;
    bool is_dataset_mode_;
};

#endif // VO_CORE_H_ 