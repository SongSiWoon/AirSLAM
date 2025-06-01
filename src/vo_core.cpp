#include "vo_core.h"

VOCore::VOCore(const VisualOdometryConfigs& configs, const rclcpp::Node::SharedPtr& node)
    : configs_(configs), node_(node), is_dataset_mode_(false) {
    std::cout << "Initializing VOCore..." << std::endl;
    try {
        std::cout << "Creating MapBuilder..." << std::endl;
        map_builder_ = std::make_shared<MapBuilder>(configs_, node_);
        std::cout << "MapBuilder created successfully" << std::endl;
    } catch (const YAML::Exception& e) {
        std::cout << "Error creating MapBuilder: " << e.what() << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cout << "Unexpected error creating MapBuilder: " << e.what() << std::endl;
        throw;
    }
}

VOCore::~VOCore() {
    Stop();
}

bool VOCore::ProcessDataset(const std::string& dataroot) {
    is_dataset_mode_ = true;
    dataset_ = std::make_shared<Dataset>(dataroot, map_builder_->UseIMU());
    
    size_t dataset_length = dataset_->GetDatasetLength();
    double sum_time = 0;
    int image_num = 0;

    // Process each frame in the dataset
    for(size_t i = 0; i < dataset_length && (!node_ || rclcpp::ok()); ++i) {
        cv::Mat image_left, image_right;
        double timestamp;
        ImuDataList batch_imu_data;
        
        if(!dataset_->GetData(i, image_left, image_right, batch_imu_data, timestamp)) 
            continue;

        // Prepare input data
        InputDataPtr data = std::shared_ptr<InputData>(new InputData());
        data->index = i;
        data->time = timestamp;
        data->image_left = image_left;
        data->image_right = image_right;
        data->batch_imu_data = batch_imu_data;

        // Process frame and measure execution time
        auto before_infer = std::chrono::high_resolution_clock::now();
        map_builder_->AddInput(data);
        auto after_infer = std::chrono::high_resolution_clock::now();
        auto cost_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_infer - before_infer).count();
        
        sum_time += (double)cost_time;
        image_num++;
    }

    return true;
}

void VOCore::ProcessImage(const cv::Mat& left_image, const cv::Mat& right_image, 
                         const ImuDataList& imu_data, double timestamp) {
    // Check if in dataset mode
    if (is_dataset_mode_) {
        if (node_) {
            RCLCPP_WARN(node_->get_logger(), "Dataset mode is active. Cannot process real-time images.");
        }
        return;
    }

    // Prepare input data for real-time processing
    static size_t frame_count = 0;
    InputDataPtr data = std::shared_ptr<InputData>(new InputData());
    data->index = frame_count++;
    data->time = timestamp;
    data->image_left = left_image;
    data->image_right = right_image;
    data->batch_imu_data = imu_data;

    map_builder_->AddInput(data);
}

void VOCore::Stop() {
    if (map_builder_) {
        map_builder_->Stop();
    }
}

bool VOCore::IsStopped() {
    return map_builder_ ? map_builder_->IsStopped() : true;
}

void VOCore::SaveTrajectory(const std::string& file_path) {
    if (map_builder_) {
        map_builder_->SaveTrajectory(file_path);
    }
}

void VOCore::SaveMap(const std::string& map_root) {
    if (map_builder_) {
        map_builder_->SaveMap(map_root);
    }
} 