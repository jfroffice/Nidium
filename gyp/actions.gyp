{
    'targets': [{
        'target_name': 'generate-private',
        'type': 'none',
        'actions': [
            {
                'action_name': 'Generating Private',
                'inputs': [
                    # Any change to dir2nvfs will trigger the action
                    '<(native_src_path)/tools/dir2nvfs.cpp',
                    '<(native_tools_path)dir2nvfs',
                    # If I use find command inside a variable, output is not 
                    # considered as multiple files using it in inputs works fine
                    '<!@(find <(native_private_dir) -type f)',
                ],
                'outputs': [
                    '<(native_private_bin)'
                ],
                'action': ['<@(native_tools_path)dir2nvfs', '<(native_private_dir)', '<@(_outputs)'],
                'process_outputs_as_sources': 1
            },
            {
                'action_name': 'Converting Private',
                'inputs': [
                    '<(native_private_bin)'
                ],
                'outputs': [
                    '<(native_private_bin_header)'
                ],
                'action': ['xxd', '-i', '<@(_inputs)', '<@(_outputs)'],
                'process_outputs_as_sources': 1
            },
            {
                'action_name': 'Renaming Private C Header',
                'inputs': [
                    '<(native_private_bin_header)'
                ],
                'outputs': [
                    ''
                ],
                'action': ['sed', '-i', '1s/.*/unsigned char private_bin[] = {/', '<@(_inputs)'],
                'process_outputs_as_sources': 1
            }
        ]
    }]
}
