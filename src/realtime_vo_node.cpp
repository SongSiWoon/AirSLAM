#include "realtime_vo_node.h"
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <yaml-cpp/yaml.h>
#include <rclcpp/qos.hpp>

RealtimeVONode::RealtimeVONode() : Node("realtime_vo_node") {
    // Declare parameters
    this->declare_parameter("config_path", "");
    this->declare_parameter("model_dir", "");
    this->declare_parameter("saving_dir", "");
    this->declare_parameter("camera_config_path", "");
    this->declare_parameter("use_imu", false);
    this->declare_parameter("max_stereo_time_diff", 0.1);
    this->declare_parameter("max_imu_time_diff", 0.1);

    // Get parameters
    config_path_ = this->get_parameter("config_path").as_string();
    model_dir_ = this->get_parameter("model_dir").as_string();
    saving_dir_ = this->get_parameter("saving_dir").as_string();
    use_imu_ = this->get_parameter("use_imu").as_bool();
    max_stereo_time_diff_ = this->get_parameter("max_stereo_time_diff").as_double();
    max_imu_time_diff_ = this->get_parameter("max_imu_time_diff").as_double();
    RCLCPP_INFO(this->get_logger(), "RealtimeVONode initialized");
    RCLCPP_INFO(this->get_logger(), "config_path: %s", config_path_.c_str());
    RCLCPP_INFO(this->get_logger(), "model_dir: %s", model_dir_.c_str());
    RCLCPP_INFO(this->get_logger(), "saving_dir: %s", saving_dir_.c_str());
    RCLCPP_INFO(this->get_logger(), "use_imu: %d", use_imu_);
    RCLCPP_INFO(this->get_logger(), "max_stereo_time_diff: %f", max_stereo_time_diff_);
    RCLCPP_INFO(this->get_logger(), "max_imu_time_diff: %f", max_imu_time_diff_);
    // Initialize VO core
    VisualOdometryConfigs configs(config_path_, model_dir_);
    configs.camera_config_path = this->get_parameter("camera_config_path").as_string();
    configs.saving_dir = saving_dir_;
    RCLCPP_INFO(this->get_logger(), "ROS Publisher config loaded successfully");
    RCLCPP_INFO(this->get_logger(), "Initializing VOCore...");
    
    try {
        RCLCPP_INFO(this->get_logger(), "Creating MapBuilder...");
        RCLCPP_INFO(this->get_logger(), "Camera config path: %s", configs.camera_config_path.c_str());
        RCLCPP_INFO(this->get_logger(), "Model dir: %s", configs.model_dir.c_str());
        
        vo_core_ = std::make_shared<VOCore>(configs, std::shared_ptr<rclcpp::Node>(this));
        RCLCPP_INFO(this->get_logger(), "VOCore initialized successfully");
    } catch (const std::exception& e) {
        RCLCPP_ERROR(this->get_logger(), "Error initializing VOCore: %s", e.what());
        throw;
    }

    // Setup subscribers
    left_image_sub_.subscribe(this, "camera/left/image_raw");
    right_image_sub_.subscribe(this, "camera/right/image_raw");

    // Setup IMU subscriber
    if (use_imu_) {
        imu_sub_ = this->create_subscription<sensor_msgs::msg::Imu>(
            "imu/data", rclcpp::SensorDataQoS(),
            std::bind(&RealtimeVONode::imuCallback, this, std::placeholders::_1));
    }

    // Setup publishers
    odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("vo/odometry", 10);
    pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("vo/pose", 10);

    // Setup stereo synchronization
    stereo_sync_.reset(new StereoSync(StereoPolicy(10), left_image_sub_, right_image_sub_));
    stereo_sync_->registerCallback(
        std::bind(&RealtimeVONode::stereoImageCallback, this, std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(this->get_logger(), "RealtimeVONode initialized");
}

cv::Mat RealtimeVONode::convertToGrayscale(const sensor_msgs::msg::Image::ConstSharedPtr& msg) {
    cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg);
    if (cv_ptr->image.channels() == 3) {
        cv::Mat gray;
        cv::cvtColor(cv_ptr->image, gray, cv::COLOR_BGR2GRAY);
        return gray;
    }
    return cv_ptr->image;
}

void RealtimeVONode::stereoImageCallback(const sensor_msgs::msg::Image::ConstSharedPtr& left_msg,
                                        const sensor_msgs::msg::Image::ConstSharedPtr& right_msg) {
    try {
        // Convert images to grayscale
        cv::Mat left_gray = convertToGrayscale(left_msg);
        cv::Mat right_gray = convertToGrayscale(right_msg);

        // Check temporal synchronization
        double time_diff = std::abs(static_cast<int64_t>(left_msg->header.stamp.sec) - static_cast<int64_t>(right_msg->header.stamp.sec)) +
                          std::abs(static_cast<int64_t>(left_msg->header.stamp.nanosec) - static_cast<int64_t>(right_msg->header.stamp.nanosec)) * 1e-9;
        if (time_diff > max_stereo_time_diff_) {
            RCLCPP_WARN(this->get_logger(), "Large time difference between stereo images: %f", time_diff);
            return;
        }

        // Process IMU data if available
        ImuDataList imu_data;
        if (use_imu_) {
            double image_time = left_msg->header.stamp.sec + left_msg->header.stamp.nanosec * 1e-9;
            imu_data = getInterpolatedImuData(image_time - max_imu_time_diff_, image_time);
        }

        // Process visual odometry
        vo_core_->ProcessImage(left_gray, right_gray, imu_data, left_msg->header.stamp.sec + left_msg->header.stamp.nanosec * 1e-9);

        // Publish results
        nav_msgs::msg::Odometry odom_msg;
        geometry_msgs::msg::PoseStamped pose_msg;
        odom_pub_->publish(odom_msg);
        pose_pub_->publish(pose_msg);

    } catch (const std::exception& e) {
        RCLCPP_ERROR(this->get_logger(), "Error processing stereo images: %s", e.what());
    }
}

void RealtimeVONode::imuCallback(const sensor_msgs::msg::Imu::SharedPtr msg) {
    ImuData imu_data;
    imu_data.timestamp = msg->header.stamp.sec + msg->header.stamp.nanosec * 1e-9;
    imu_data.gyr = Eigen::Vector3d(msg->angular_velocity.x,
                                   msg->angular_velocity.y,
                                   msg->angular_velocity.z);
    imu_data.acc = Eigen::Vector3d(msg->linear_acceleration.x,
                                    msg->linear_acceleration.y,
                                    msg->linear_acceleration.z);

    imu_data_.push_back(imu_data);

    // Remove old IMU data
    while (!imu_data_.empty() &&
           imu_data_.front().timestamp < last_image_time_ - max_imu_time_diff_) {
        imu_data_.pop_front();
    }
}

ImuDataList RealtimeVONode::getInterpolatedImuData(double start_time, double end_time) {
    ImuDataList result;
    if (imu_data_.empty()) return result;

    // Find IMU data between start and end time
    auto it = imu_data_.begin();
    while (it != imu_data_.end() && it->timestamp < start_time) ++it;
    
    if (it == imu_data_.end()) return result;

    // Previous data for linear interpolation
    ImuData prev_data = *it;
    ++it;

    // Interpolate data
    while (it != imu_data_.end() && it->timestamp <= end_time) {
        double alpha = (it->timestamp - prev_data.timestamp);
        if (alpha > 0) {
            ImuData interpolated;
            interpolated.timestamp = (start_time + end_time) / 2.0;
            interpolated.gyr = prev_data.gyr + 
                (it->gyr - prev_data.gyr) * alpha;
            interpolated.acc = prev_data.acc + 
                (it->acc - prev_data.acc) * alpha;
            result.push_back(interpolated);
        }
        prev_data = *it;
        ++it;
    }

    return result;
}

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<RealtimeVONode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
} 