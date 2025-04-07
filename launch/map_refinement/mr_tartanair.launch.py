from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    # Declare arguments
    config_path = DeclareLaunchArgument(
        'config_path',
        default_value='air_slam/configs/map_refinement/mr_tartanair.yaml'
    )
    map_root = DeclareLaunchArgument(
        'map_root',
        default_value='/media/code/ubuntu_files/airvio/experiments/results/tartanair/abandonedfactory/maps/P009'
    )
    model_dir = DeclareLaunchArgument(
        'model_dir',
        default_value='air_slam/output'
    )
    voc_path = DeclareLaunchArgument(
        'voc_path',
        default_value='air_slam/voc/point_voc_L4.bin'
    )
    breakpoint_arg = DeclareLaunchArgument(
        'breakpoint',
        default_value='0'
    )
    visualization = DeclareLaunchArgument(
        'visualization',
        default_value='true'
    )

    # Main map_refinement node
    map_refinement_node = Node(
        package='air_slam',
        executable='map_refinement',
        name='map_refinement',
        output='screen',
        parameters=[{
            'config_path': LaunchConfiguration('config_path'),
            'map_root': LaunchConfiguration('map_root'),
            'model_dir': LaunchConfiguration('model_dir'),
            'voc_path': LaunchConfiguration('voc_path'),
            'breakpoint': LaunchConfiguration('breakpoint')
        }]
    )

    # Optional RViz launch
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz',
        arguments=['-d', 'air_slam/rviz/map_optimization.rviz'],
        output='screen',
        condition=IfCondition(LaunchConfiguration('visualization'))
    )

    return LaunchDescription([
        config_path,
        map_root,
        model_dir,
        voc_path,
        breakpoint_arg,
        visualization,
        map_refinement_node,
        rviz_node
    ])
