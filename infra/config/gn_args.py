# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Named GN-arg configs, composed by builders in `builders/ci/*.py`.

Values here are reverse-engineered from a real `linux-x64-asan-brave` nightly
run (captured `args.gn`/`.env`), not from reading `buildArgs.ts` in the
abstract, so they hold for that job today. A wider audit across all of
`config.ts`'s `envConfig.*` call sites (see the ASan migration design doc's
Appendix C) will likely split some of these further as more builders are
added.
"""

from lib.config import gn_args

gn_args.config(name='x64', args={
    'target_cpu': 'x64',
})

gn_args.config(
    name='linux',
    args={
        'target_os': 'linux',
        'symbol_level': 1,
        'use_debug_fission': True,
        'use_vaapi': True,
        'is_universal_binary': False,
        'enable_cdm_host_verification': False,
        'skip_signing': True,
    },
)

gn_args.config(name='release',
               args={
                   'is_debug': False,
                   'is_component_build': False,
               })

gn_args.config(
    name='remoteexec',
    args={
        'use_remoteexec': True,
        'use_siso': True,
        'use_reclient': False,
    },
)

gn_args.config(name='nightly', args={'brave_channel': 'nightly'})

# brave/build/args/brave_defaults.gni
gn_args.config(
    name='brave_defaults',
    args={
        'disable_fieldtrial_testing_config': True,
        'translate_genders': False,
        'enable_precompiled_headers': False,
        'enable_pseudolocales': False,
        'ignore_missing_widevine_signing_cert': False,
        'root_extra_deps': [
            '//brave',
        ],
    },
)

# brave/build/args/blink_platform_defaults.gni (non-iOS only)
gn_args.config(
    name='blink_platform_defaults',
    args={
        'proprietary_codecs': True,
        'ffmpeg_branding': 'Chrome',
        'enable_widevine': True,
        'enable_platform_hevc': True,
        'enable_hevc_parser_and_hw_decoder': True,
    },
)

# brave/build/args/branding_defaults.gni
gn_args.config(
    name='branding_defaults',
    args={
        'branding_path_component': 'brave',
        'branding_path_product': 'brave',
    },
)

# brave/build/args/desktop_defaults.gni (non-mobile only)
gn_args.config(name='desktop_defaults', args={
    'safe_browsing_mode': 1,
})

gn_args.config(
    name='brave_desktop_defaults',
    configs=[
        'brave_defaults',
        'blink_platform_defaults',
        'branding_defaults',
        'desktop_defaults',
    ],
)

# What buildArgs.ts derives for any non-branded desktop CI build. Not split
# further yet since every builder we have so far is this shape.
gn_args.config(
    name='ci_desktop_defaults',
    args={
        'devtools_skip_typecheck': False,
        'enable_hangout_services_extension': False,
        'use_libfuzzer': False,
    },
)

gn_args.config(
    name='brave_service_keys',
    args={
        # Unconditionally "dummy" today regardless of secrets availability.
        'brave_google_api_key': 'dummy',
    },
    secrets={
        'brave_services_key': 'BRAVE_SERVICES_KEY',
        'brave_stats_api_key': 'BRAVE_STATS_API_KEY',
        'google_default_client_id': 'GOOGLE_OAUTH_CLIENT_ID',
        'google_default_client_secret': 'GOOGLE_OAUTH_CLIENT_SECRET',
        # These three are are not env values actually, but we are going to
        # handle them later on.
        'service_key_aichat': 'SERVICE_KEY_AICHAT',
        'service_key_search': 'SERVICE_KEY_SEARCH',
        'service_key_stt': 'SERVICE_KEY_STT',
    },
)

gn_args.config(
    name='asan',
    configs=[
        'brave_desktop_defaults',
        'ci_desktop_defaults',
        'brave_service_keys',
    ],
    args={
        'is_asan': True,
        'is_lsan': True,
        'enable_full_stack_frames_for_profiling': True,
        'v8_enable_verify_heap': True,
        'is_official_build': False,
        'enable_update_notifications': False,
        'dcheck_always_on': False,
    },
)
