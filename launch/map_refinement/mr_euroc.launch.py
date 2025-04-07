import launch
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo, SetEnvironmentVariable
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    return LaunchDescription([
        # Define launch arguments
        DeclareLaunchArgument('config_path', default_value='$(find air_slam)/configs/map_refinement/mr_euroc.yaml'),
        DeclareLaunchArgument('map_root', default_value='$(find air_slam)/debug/test'),
        DeclareLaunchArgument('model_dir', default_value='$(find air_slam)/output'),
        DeclareLaunchArgument('voc_path', default_value='$(find air_slam)/voc/point_voc_L4.bin'),
        DeclareLaunchArgument('breakpoint', default_value='0'),
        DeclareLaunchArgument('visualization', default_value='true'),

        # Launch map_refinement node
        Node(
            package='air_slam',
            executable='map_refinement',
            name='map_refinement',
            output='screen',
            parameters=[
                {'config_path': LaunchConfiguration('config_path')},
                {'map_root': LaunchConfiguration('map_root')},
                {'model_dir': LaunchConfiguration('model_dir')},
                {'voc_path': LaunchConfiguration('voc_path')},
                {'breakpoint': LaunchConfiguration('breakpoint')},
            ],
            # You can add remappings, arguments etc. here if needed
        ),

        # Visualization node (RViz) - conditional execution
        launch.actions.OpaqueFunction(function=lambda context: LogInfo("Visualization Enabled"))
        if LaunchConfiguration('visualization') == 'true' else None,

        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz',
            output='screen',
            arguments=['-d', LaunchConfiguration('rviz_config')],
            condition=launch.conditions.IfCondition(LaunchConfiguration('visualization'))
        ),
    ])
