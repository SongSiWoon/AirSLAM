#ifndef REALTIME_VO_NODE_H_
#define REALTIME_VO_NODE_H_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <cv_bridge/cv_bridge.h>
#include "message_filters/subscriber.h"
#include "message_filters/synchronizer.h"
#include "message_filters/sync_policies/approximate_time.h"
#include <memory>
#include <atomic>

#include "vo_core.h"

class RealtimeVONode : public rclcpp::Node {
public:
    // Factory method to create shared_ptr instance
    static std::shared_ptr<RealtimeVONode> create();
    
    void initialize();

private:
    RealtimeVONode();  // Private constructor
    // Callback functions for sensor data
    void stereoImageCallback(const sensor_msgs::msg::Image::ConstSharedPtr& left_msg,
                            const sensor_msgs::msg::Image::ConstSharedPtr& right_msg);
    void imuCallback(const sensor_msgs::msg::Imu::SharedPtr msg);
    ImuDataList getInterpolatedImuData(double start_time, double end_time);
    cv::Mat convertToGrayscale(const sensor_msgs::msg::Image::ConstSharedPtr& msg);

    // ROS2 subscribers and publishers
    message_filters::Subscriber<sensor_msgs::msg::Image> left_image_sub_;
    message_filters::Subscriber<sensor_msgs::msg::Image> right_image_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_pub_;
    
    // Stereo synchronization policy
    using StereoPolicy = message_filters::sync_policies::ApproximateTime<sensor_msgs::msg::Image, sensor_msgs::msg::Image>;
    using StereoSync = message_filters::Synchronizer<StereoPolicy>;
    std::shared_ptr<StereoSync> stereo_sync_;
    
    // Visual odometry core
    std::shared_ptr<VOCore> vo_core_;
    
    // Image and IMU data buffers
    cv::Mat left_image_;
    cv::Mat right_image_;
    std::deque<ImuData> imu_data_;  // Changed from vector to deque for efficient front/back operations
    double last_image_time_;
    
    // Configuration storage
    VisualOdometryConfigs configs_;
    
    // Synchronization parameters
    double max_stereo_time_diff_;  // Maximum time difference between stereo images
    double max_imu_time_diff_;     // Maximum time difference between IMU data and images
    
    // Configuration parameters
    std::string config_path_;
    std::string model_dir_;
    std::string saving_dir_;
    bool use_imu_;
    
    // Thread safety flag
    std::atomic<bool> is_processing_;
};

#endif // REALTIME_VO_NODE_H_ 