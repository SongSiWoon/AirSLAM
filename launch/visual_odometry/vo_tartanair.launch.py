from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # Declare arguments
        DeclareLaunchArgument('config_path', default_value='air_slam/configs/visual_odometry/configs_tartanair.yaml'),
        DeclareLaunchArgument('dataroot', default_value='/media/bssd/datasets/tartanair/euroc_style/with_time/abandonedfactory/P009'),
        DeclareLaunchArgument('camera_config_path', default_value='air_slam/configs/camera/tartanair.yaml'),
        DeclareLaunchArgument('model_dir', default_value='air_slam/output'),
        DeclareLaunchArgument('saving_dir', default_value='air_slam/debug'),
        DeclareLaunchArgument('visualization', default_value='true'),

        # Main VO Node
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

        # RViz2 Node (optional)
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz',
            arguments=['-d', 'air_slam/rviz/vo.rviz'],
            output='screen',
            condition=IfCondition(LaunchConfiguration('visualization'))
        )
    ])
