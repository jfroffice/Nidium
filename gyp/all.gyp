{
    'targets': [{
        'target_name': 'native',
        'type': 'none',
        'includes': [
            'common.gypi'
        ],
        'dependencies': [
            #'third-party.gyp:third-party',
            'network.gyp:nativenetwork',
            'interface.gyp:nativeinterface',
            'native.gyp:nativestudio',
            'app.gyp:nativeapp',
        ],
        'conditions': [
            ['native_audio==1', {
                'dependencies': [
                    'av.gyp:nativeav'
                 ]
            }]
         ]
    }]
}
