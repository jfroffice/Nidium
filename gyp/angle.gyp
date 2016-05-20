# Copyright 2016 Nidium Inc. All rights reserved.
# Use of this source code is governed by a MIT license
# that can be found in the LICENSE file.

{
    'target_defaults': {
        'defines': [
            'NOMINMAX',
        ]
    },
    'variables': {
        'component': 'static_library'
    },
    'includes': [
        # Can't use variable in includes 
        # https://code.google.com/p/gyp/wiki/InputFormatReference#Processing_Order
        '../third-party/angle/src/build_angle.gypi'
       #'../third-party/angle/src/angle.gypi'
    ],
    'targets': [{
        'target_name': 'angle',
        'type': 'none',
        'direct_dependent_settings': {
            'include_dirs': [
                '<(third_party_path)/angle/include/'
            ]
        }
    }]
}

