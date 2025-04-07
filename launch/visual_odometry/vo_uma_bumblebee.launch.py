from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument('config_path', default_value='air_slam/configs/visual_odometry/vo_uma_bumblebee.yaml'),
        DeclareLaunchArgument('dataroot', default_value='/media/data/datasets/uma/selected_seq/indoor/third-floor-csc1_2019-03-04-20-06-37_IllChange'),
        DeclareLaunchArgument('camera_config_path', default_value='air_slam/configs/camera/uma_bumblebee.yaml'),
        DeclareLaunchArgument('model_dir', default_value='air_slam/output'),
        DeclareLaunchArgument('saving_dir', default_value='air_slam/debug'),
        DeclareLaunchArgument('visualization', default_value='true'),

        Node(
            package='air_slam',
            executable='visual_odometry',
            name='visual_odometry',
            output='screen',
            parameters=[{
                'config_path': LaunchConfiguration('config_path'),
                'dataroot': LaunchConfiguration('dataroot'),
                'camera_config_path': LaunchConfiguration('camera_config_path'),
                'model_dir': LaunchConfiguration('model_dir'),
                'saving_dir': LaunchConfiguration('saving_dir'),
            }]
        ),

        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz',
            arguments=['-d', 'air_slam/rviz/vo.rviz'],
            output='screen',
            condition=IfCondition(LaunchConfiguration('visualization'))
        )
    ])
