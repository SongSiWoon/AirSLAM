from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # Declare arguments
        DeclareLaunchArgument('config_path', default_value='air_slam/configs/relocalization/reloc_euroc.yaml'),
        DeclareLaunchArgument('dataroot', default_value='/media/data/datasets/euroc/seq/V1_01_easy/cam0/data'),
        DeclareLaunchArgument('camera_config_path', default_value='air_slam/configs/camera/euroc.yaml'),
        DeclareLaunchArgument('model_dir', default_value='air_slam/output'),
        DeclareLaunchArgument('traj_path', default_value='air_slam/debug/relocalization.txt'),
        DeclareLaunchArgument('voc_path', default_value='air_slam/voc/point_voc_L4.bin'),
        DeclareLaunchArgument('map_root', default_value='air_slam/debug/test'),
        DeclareLaunchArgument('visualization', default_value='true'),

        # Main relocalization node
        Node(
            package='air_slam',
            executable='relocalization',
            name='relocalization',
            output='screen',
            parameters=[{
                'config_path': LaunchConfiguration('config_path'),
                'dataroot': LaunchConfiguration('dataroot'),
                'camera_config_path': LaunchConfiguration('camera_config_path'),
                'model_dir': LaunchConfiguration('model_dir'),
                'traj_path': LaunchConfiguration('traj_path'),
                'voc_path': LaunchConfiguration('voc_path'),
                'map_root': LaunchConfiguration('map_root')
            }]
        ),

        # Optional RViz node
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz',
            arguments=['-d', 'air_slam/rviz/relocalization.rviz'],
            output='screen',
            condition=IfCondition(LaunchConfiguration('visualization'))
        )
    ])
