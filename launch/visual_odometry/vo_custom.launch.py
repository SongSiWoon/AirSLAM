import os
from pathlib import Path                   # ← 가독성 좋은 Path 사용 권장
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument, GroupAction
)
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_air_slam = Path(__file__).resolve().parents[2]   # <pkg>/launch/vo.launch.py → <pkg>

    default_cfg       = pkg_air_slam / 'configs/visual_odometry/custom.yaml'
    default_cam_cfg   = pkg_air_slam / 'configs/camera/realsense_d455.yaml'
    default_model_dir = pkg_air_slam / 'output'
    default_save_dir  = pkg_air_slam / 'debug'
    default_rviz_cfg  = pkg_air_slam / 'rviz/vo.rviz'
    default_dataroot  = pkg_air_slam / 'dataset/06.16_rtk_01/stereo_dataset'

    # LaunchConfiguration로 선언한 인자는 반드시 DeclareLaunchArgument가 필요
    return LaunchDescription([
        DeclareLaunchArgument('config_path',       default_value=str(default_cfg)),
        DeclareLaunchArgument('dataroot',          default_value=str(default_dataroot)),
        DeclareLaunchArgument('camera_config_path',default_value=str(default_cam_cfg)),
        DeclareLaunchArgument('model_dir',         default_value=str(default_model_dir)),
        DeclareLaunchArgument('saving_dir',        default_value=str(default_save_dir)),
        DeclareLaunchArgument('visualization',     default_value='false'),
        DeclareLaunchArgument('log_level',         default_value='info'),

        Node(
            package='air_slam',
            executable='visual_odometry',
            name='visual_odometry',
            output='screen',
            parameters=[{
                'config_path'       : LaunchConfiguration('config_path'),
                'dataroot'          : LaunchConfiguration('dataroot'),
                'camera_config_path': LaunchConfiguration('camera_config_path'),
                'model_dir'         : LaunchConfiguration('model_dir'),
                'saving_dir'        : LaunchConfiguration('saving_dir'),
            }],
            arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')]
        ),

        GroupAction([
            Node(
                package='rviz2',
                executable='rviz2',
                name='rviz2',
                arguments=['-d', str(default_rviz_cfg)],
                output='screen'
            )
        ], condition=IfCondition(LaunchConfiguration('visualization')))
    ])
