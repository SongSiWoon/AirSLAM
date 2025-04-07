from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # Launch arguments
        DeclareLaunchArgument('config_path', default_value='air_slam/configs/relocalization/reloc_tartanair.yaml'),
        DeclareLaunchArgument('dataroot', default_value='/media/bssd/datasets/tartanair/mapping_relocalization/relocalization/abandonedfactory/sequences/P000'),
        DeclareLaunchArgument('camera_config_path', default_value='air_slam/configs/camera/tartanair.yaml'),
        DeclareLaunchArgument('model_dir', default_value='air_slam/output'),
        DeclareLaunchArgument('traj_path', default_value='/media/bssd/datasets/tartanair/mapping_relocalization/results/tmp/relo_results/P000.txt'),
        DeclareLaunchArgument('voc_path', default_value='air_slam/voc/point_voc_L4.bin'),
        DeclareLaunchArgument('map_root', default_value='/media/code/ubuntu_files/airvio/experiments/results/tartanair/abandonedfactory/maps/P000'),
        DeclareLaunchArgument('visualization', default_value='true'),

        # Main node
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

        # RViz node (optional)
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz',
            arguments=['-d', 'air_slam/rviz/relocalization.rviz'],
            output='screen',
            condition=IfCondition(LaunchConfiguration('visualization'))
        )
    ])
