from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # Declare launch arguments
        DeclareLaunchArgument(
            'config_path',
            default_value='air_slam/configs/map_refinement/mr_oivio.yaml',
            description='Path to map refinement config file'
        ),
        DeclareLaunchArgument(
            'map_root',
            default_value='/media/data/datasets/oivio/results/air_slam/maps/TN_100_GV_01',
            description='Path to saved map directory'
        ),
        DeclareLaunchArgument(
            'model_dir',
            default_value='air_slam/output',
            description='Path to model directory'
        ),
        DeclareLaunchArgument(
            'voc_path',
            default_value='air_slam/voc/point_voc_L4.bin',
            description='Path to vocabulary file'
        ),
        DeclareLaunchArgument(
            'breakpoint',
            default_value='0',
            description='Breakpoint flag'
        ),
        DeclareLaunchArgument(
            'visualization',
            default_value='true',
            description='Whether to launch RViz'
        ),

        # Map refinement node
        Node(
            package='air_slam',
            executable='map_refinement',
            name='map_refinement',
            output='screen',
            parameters=[{
                'config_path': LaunchConfiguration('config_path'),
                'map_root': LaunchConfiguration('map_root'),
                'model_dir': LaunchConfiguration('model_dir'),
                'voc_path': LaunchConfiguration('voc_path'),
                'breakpoint': LaunchConfiguration('breakpoint'),
            }]
        ),

        # Optional RViz
        GroupAction([
            Node(
                package='rviz2',
                executable='rviz2',
                name='rviz2',
                arguments=['-d', 'air_slam/rviz/map_optimization.rviz'],
                output='screen'
            )
        ], condition=IfCondition(LaunchConfiguration('visualization')))
    ])
