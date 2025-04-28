cmake_minimum_required(VERSION 3.5)
project(air_slam)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
add_definitions(-w)

# Thirdparty
add_subdirectory(${PROJECT_SOURCE_DIR}/3rdparty/tensorrtbuffer)
add_subdirectory(${PROJECT_SOURCE_DIR}/3rdparty/DBoW2)

# ROS2
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rmw REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(image_transport REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(rcl_interfaces REQUIRED)

# External libraries
find_package(OpenCV REQUIRED COMPONENTS core imgproc ximgproc)
find_package(Eigen3 REQUIRED)
find_package(CUDA REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED)
find_package(G2O REQUIRED)
find_package(Gflags REQUIRED)
find_package(Glog REQUIRED)

# Include directories
include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${OpenCV_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIRS}
  ${CUDA_INCLUDE_DIRS}
  ${YAML_CPP_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${G2O_INCLUDE_DIR}
  ${GFLAGS_INCLUDE_DIRS}
  ${GLOG_INCLUDE_DIRS}
)

# Main Library
add_library(${PROJECT_NAME}_lib SHARED
  src/g2o_optimization/vertex_line3d.cc
  src/g2o_optimization/vertex_vi_pose.cc
  src/g2o_optimization/vertex_imu.cc
  src/g2o_optimization/edge_project_point.cc
  src/g2o_optimization/edge_project_line.cc
  src/g2o_optimization/edge_imu.cc
  src/g2o_optimization/edge_relative_pose.cc
  src/g2o_optimization/g2o_optimization.cc
  src/bow/FSuperpoint.cc
  src/bow/database.cc
  src/super_point.cpp
  src/feature_detector.cc
  src/super_glue.cpp
  src/light_glue.cpp
  src/plnet.cpp
  src/utils.cc
  src/camera.cc
  src/imu.cc
  src/dataset.cc
  src/frame.cc
  src/point_matcher.cc
  src/mappoint.cc
  src/mapline.cc
  src/line_processor.cc
  src/ros_publisher.cc
  src/map.cc
  src/map_builder.cc
  src/map_refiner.cc
  src/map_user.cc
  src/timer.cc
  src/debug.cc
)

ament_target_dependencies(${PROJECT_NAME}_lib
  rclcpp
  rmw
  rosidl_runtime_c
  sensor_msgs
  std_msgs
  nav_msgs
  geometry_msgs
  visualization_msgs
  cv_bridge
  image_transport
  rcl_interfaces
  tf2_ros
)

target_link_libraries(${PROJECT_NAME}_lib
  nvinfer
  nvonnxparser
  ${OpenCV_LIBRARIES}
  ${CUDA_LIBRARIES}
  ${Boost_LIBRARIES}
  ${G2O_LIBRARIES}
  ${GFLAGS_LIBRARIES}
  ${GLOG_LIBRARIES}
  yaml-cpp
  tensorrtbuffer
  DBoW2
  -lboost_serialization
)

# Executables
add_executable(visual_odometry demo/visual_odometry.cpp)
target_link_libraries(visual_odometry ${PROJECT_NAME}_lib)

add_executable(map_refinement demo/map_refinement.cpp)
target_link_libraries(map_refinement ${PROJECT_NAME}_lib)

add_executable(relocalization demo/relocalization.cpp)
target_link_libraries(relocalization ${PROJECT_NAME}_lib)

add_executable(test_feature demo/test_feature.cpp)
target_link_libraries(test_feature ${PROJECT_NAME}_lib)

# Install
install(TARGETS
  ${PROJECT_NAME}_lib
  visual_odometry
  map_refinement
  relocalization
  test_feature
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include/
)

install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}
)

# Export
ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME}_lib)
ament_export_dependencies(
  rclcpp
  rmw
  rosidl_runtime_c
  sensor_msgs
  std_msgs
  nav_msgs
  geometry_msgs
  visualization_msgs
  cv_bridge
  image_transport
  rcl_interfaces
  tf2_ros
)

ament_package()
