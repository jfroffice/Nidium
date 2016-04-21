{
    'targets': [{
        'target_name': '<(native_exec_name)',
        'type': 'executable',
        'product_dir': '<(native_exec_path)',
        'dependencies': [
            '<(native_nativejscore_path)/gyp/nativejscore.gyp:*',
            '<(native_network_path)/gyp/network.gyp:*'
        ],
        'include_dirs': [
            '<(native_network_path)',
            '<(third_party_path)/mozilla-central/js/src/dist/include/',
            '<(third_party_path)/http-parser/',
        ],
        'sources': [
            '<(native_src_path)/native_main.cpp',
            '<(native_src_path)/Server.cpp',
            '<(native_src_path)/Context.cpp',
            '<(native_src_path)/NativeJSConsole.cpp',
            '<(native_src_path)/REPL.cpp',
            '<(native_src_path)/JSSystem.cpp',
            '<(native_src_path)/external/setproctitle.c',
            '<(native_src_path)/external/linenoise.c',

        ],
        'conditions': [
            ['nofork==1', {
                'defines':['NATIVE_NO_FORK']
            }],
            ['OS=="linux"', {
                'cflags': [
                    '-fno-rtti',
                    # Not sure this makes any big difference. At least here.
                    # Could be better in network common.gypi 
                    # See :http://stackoverflow.com/questions/4274804/query-on-ffunction-section-fdata-sections-options-of-gcc
                    #'-ffunction-sections',
                    #'-fdata-sections',
                    #'-fno-exceptions',
                    #'-freorder-blocks',
                    # Not sure this is interesting for us
                    # See : http://stackoverflow.com/questions/1942801/when-should-i-omit-the-frame-pointer
                    #'-fomit-frame-pointer',
                ],
            }],
        ],
    }]
}
