import os

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    # air_slam 패키지의 share 디렉토리
    pkg_air_slam = os.path.dirname(os.path.realpath(__file__))

    # 경로 설정
    default_config_path = os.path.join(pkg_air_slam, '../../configs/visual_odometry/vo_euroc.yaml')
    default_camera_config_path = os.path.join(pkg_air_slam, '../../configs/camera/euroc.yaml')
    default_model_dir = os.path.join(pkg_air_slam, '../../output')
    default_saving_dir = os.path.join(pkg_air_slam, 'debug')
    default_rviz_config = os.path.join(pkg_air_slam, '../../rviz/vo.rviz')

    return LaunchDescription([
        # Declare launch arguments
        DeclareLaunchArgument(
            'config_path',
            default_value=default_config_path,
            description='Path to config yaml'
        ),
        DeclareLaunchArgument(
            'dataroot',
            default_value='/media/data/datasets/euroc/seq/V1_02_medium',
            description='Dataset root path'
        ),
        DeclareLaunchArgument(
            'camera_config_path',
            default_value=default_camera_config_path,
            description='Path to camera config'
        ),
        DeclareLaunchArgument(
            'model_dir',
            default_value=default_model_dir,
            description='Model directory'
        ),
        DeclareLaunchArgument(
            'saving_dir',
            default_value=default_saving_dir,
            description='Where to save debug results'
        ),
        DeclareLaunchArgument(
            'visualization',
            default_value='true',
            description='Whether to launch RViz'
        ),

        # Main VO node
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

        # Optional RViz visualization
        GroupAction([
            Node(
                package='rviz2',
                executable='rviz2',
                name='rviz2',
                arguments=['-d', default_rviz_config],
                output='screen'
            )
        ], condition=IfCondition(LaunchConfiguration('visualization')))
    ])
