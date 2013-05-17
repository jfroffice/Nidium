{
    'targets': [{
        'target_name': 'nativestudio',
        'type': 'static_library',
        'include_dirs': [
            '<(native_src_path)',
            '<(third_party_path)/mozilla-central/js/src/dist/include/',
            '<(third_party_path)/skia/',
            '<(third_party_path)/skia/include/core',
            '<(third_party_path)/skia/include/pipe',
            '<(third_party_path)/skia/include/gpu',
            '<(third_party_path)/skia/include/utils',
            '<(third_party_path)/skia/include/device',
            '<(third_party_path)/skia/include/config',
            '<(third_party_path)/skia/include/images',
            '<(third_party_path)/skia/include/test',
            '<(third_party_path)/skia/include/pdf',
            '<(third_party_path)/skia/src/gpu/gl',
            '<(third_party_path)/skia/src/gpu',
            '<(third_party_path)/skia/src/core',
            '<(third_party_path)/skia/include/effects',
            '<(third_party_path)/skia/include/utils/mac',
            '<(third_party_path)/skia/include/lazy',
            '<(native_interface_path)/',
            '<(native_network_path)/',
            '<(third_party_path)/http-parser',
            '<(native_av_path)',
            '<(third_party_path)/libzip/lib',
            '<(third_party_path)/jsoncpp/include',
            '<(third_party_path)/ffmpeg/',
            '<(third_party_path)/libcoroutine/source/',
            '<(third_party_path)/basekit/source/',
        ],
        'conditions': [
            ['OS=="mac"', {
                'defines': [
                    'ENABLE_TYPEDARRAY_MOVE',
                    'ENABLE_YARR_JIT=1',
                    'NO_NSPR_10_SUPPORT',
                    'IMPL_MFBT EXPORT_JS_API',
                    'MOZILLA_CLIENT',
                    'SK_GAMMA_SRGB',
                    'SK_GAMMA_APPLY_TO_A8',
                    'SK_ALLOW_STATIC_GLOBAL_INITIALIZERS=1',
                    'SK_SCALAR_IS_FLOAT',
                    'SK_CAN_USE_FLOAT',
                    'SK_SUPPORT_GPU=1',
                    'SK_BUILD_FOR_MAC',
                    'SK_USE_POSIX_THREADS',
                    'GR_MAC_BUILD=1',
                    'SK_SUPPORT_PDF',
                    'SK_RELEASE',
                    'GR_RELEASE=1',
                    'TRACING',
                    'JS_THREADSAFE',
                ],
				'xcode_settings': {
					'OTHER_CFLAGS': [
                        '-Qunused-arguments',
                        '-fvisibility=hidden',
                        '-fvisibility-inlines-hidden',
                        '-Wno-c++0x-extensions',
                        '-Wno-unused-function',
                        '-Wno-invalid-offsetof'
					],
				},
			}],
            ['OS=="linux"', {
                'defines+': [
                    'EXPORT_JS_API',
                    'IMPL_MFBT',
                    'USE_SYSTEM_MALLOC=1',
                    'ENABLE_ASSEMBLER=1',
                    'ENABLE_JIT=1',
                    'EXPORT_JS_API',
                    'IMPL_MFBT',
                    'SK_GAMMA_SRGB',
                    'SK_GAMMA_APPLY_TO_A8',
                    'SK_ALLOW_STATIC_GLOBAL_INITIALIZERS=1',
                    'SK_SCALAR_IS_FLOAT',
                    'SK_CAN_USE_FLOAT',
                    'SK_SUPPORT_GPU=1',
                    'SK_SAMPLES_FOR_X',
                    'SK_BUILD_FOR_UNIX',
                    'SK_USE_POSIX_THREADS',
                    'SK_SUPPORT_PDF',
                    'GR_LINUX_BUILD=1',
                    'SK_RELEASE',
                    'UINT32_MAX=4294967295u',
                    'GR_RELEASE=1',
                    '__STDC_CONSTANT_MACROS'
                ],
                'cflags+': [
                    '-fvisibility=hidden',
                    '-fvisibility-inlines-hidden',
                    '-Wno-c++0x-extensions',
                    '-Wno-unused-function',
                    '-Wno-invalid-offsetof'
                ],
            }],
            ['native_audio==1', {
                'sources': [
                    '<(native_src_path)/NativeJSAV.cpp',
                 ],
                 'defines+': [ 'NATIVE_AUDIO_ENABLED' ],
                 'dependencies': [
                    'av.gyp:nativeav'
                 ]
            }],
            ['native_webgl==1', {
                'sources': [
                    '<(native_src_path)/NativeJSWebGL.cpp',
                 ],
                 'defines+': [ 'NATIVE_WEBGL_ENABLED' ],
                 'dependencies': [
                    'angle.gyp:*'
                 ]
            }],
        ],
        'sources': [
            '<(native_src_path)/NativeJS.cpp',
            '<(native_src_path)/NativeUtils.cpp',
            '<(native_src_path)/NativeSkia.cpp',
            '<(native_src_path)/NativeSkGradient.cpp',
            '<(native_src_path)/NativeSkImage.cpp',
            '<(native_src_path)/NativeShadowLooper.cpp',
            '<(native_src_path)/NativeSharedMessages.cpp',
            '<(native_src_path)/NativeJSExposer.cpp',
            '<(native_src_path)/NativeJSSocket.cpp',
            '<(native_src_path)/NativeJSThread.cpp',
            '<(native_src_path)/NativeHTTP.cpp',
            '<(native_src_path)/NativeJSHttp.cpp',
            '<(native_src_path)/NativeJSImage.cpp',
            '<(native_src_path)/NativeJSNative.cpp',
            '<(native_src_path)/NativeFileIO.cpp',
            '<(native_src_path)/NativeJSWindow.cpp',
            '<(native_src_path)/NativeJSCanvas.cpp',
            '<(native_src_path)/NativeCanvasHandler.cpp',
            '<(native_src_path)/NativeCanvas2DContext.cpp',
            '<(native_src_path)/NativeJSFileIO.cpp',
            '<(native_src_path)/NativeStream.cpp',
            '<(native_src_path)/NativeApp.cpp',
            '<(native_src_path)/NativeJSAV.cpp',
		    '<(native_src_path)/NativeJSConsole.cpp',
			'<(native_src_path)/NativeNML.cpp',
			'<(native_src_path)/NativeAssets.cpp',
			'<(native_src_path)/NativeJSModules.cpp',
        ],
    }],
}
