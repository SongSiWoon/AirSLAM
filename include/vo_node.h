#ifndef VO_NODE_H_
#define VO_NODE_H_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <cv_bridge/cv_bridge.h>
#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#include "vo_core.h"

class VONode : public rclcpp::Node {
public:
    VONode();

private:
    // Image processing callbacks
    void leftImageCallback(const sensor_msgs::msg::Image::SharedPtr msg);
    void rightImageCallback(const sensor_msgs::msg::Image::SharedPtr msg);
    void imuCallback(const sensor_msgs::msg::Imu::SharedPtr msg);
    void stereoImageCallback(const sensor_msgs::msg::Image::SharedPtr left_msg,
                            const sensor_msgs::msg::Image::SharedPtr right_msg);
    
    // Core processing functions
    void processImages();
    void publishResults();
    ImuDataList getInterpolatedImuData(double start_time, double end_time);

    // ROS2 subscribers and publishers
    message_filters::Subscriber<sensor_msgs::msg::Image> left_image_sub_;
    message_filters::Subscriber<sensor_msgs::msg::Image> right_image_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    
    // Stereo synchronization policy
    typedef message_filters::sync_policies::ApproximateTime<sensor_msgs::msg::Image, sensor_msgs::msg::Image> StereoPolicy;
    typedef message_filters::Synchronizer<StereoPolicy> StereoSync;
    std::shared_ptr<StereoSync> stereo_sync_;
    
    // Visual odometry core
    std::shared_ptr<VOCore> vo_core_;
    
    // Image and IMU data buffers
    cv::Mat left_image_;
    cv::Mat right_image_;
    ImuDataList imu_data_;
    double last_image_time_;
    bool has_left_image_;
    bool has_right_image_;
    
    // Synchronization parameters
    double max_stereo_time_diff_;  // Maximum time difference between stereo images
    double max_imu_time_diff_;     // Maximum time difference between IMU and image data
    
    // Configuration parameters
    std::string config_path_;
    std::string model_dir_;
    std::string saving_dir_;
    bool use_imu_;
};

#endif // VO_NODE_H_ 