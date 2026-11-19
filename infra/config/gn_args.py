# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Named GN-arg configs, composed by builders in `builders/*.py`.

The configs with a `defaults_file` are the defaults every Brave build imports.
`bots.py snapshot` writes each to `generated/defaults/`, where
`build/commands/lib/buildArgs.ts` imports them from, and builders compose them
like any other config. The other values here are those of a real
`linux-x64-asan-brave` nightly run, from its `args.gn` and `.env`.
"""

from lib.config import gn_args

gn_args.config(
    name='x64',
    args={
        'target_cpu': 'x64',
    },
)

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

gn_args.config(
    name='release',
    args={
        'is_debug': False,
        'is_component_build': False,
    },
)

gn_args.config(
    name='remoteexec',
    args={
        'use_remoteexec': True,
        'use_siso': True,
    },
)

# The release channel this branch builds. Switch it in the channel
# migration, then run `bots.py snapshot`: nightly on master; beta on the
# branch one version behind it, and release on those further behind.
gn_args.config(name='channel', args={'brave_channel': 'nightly'})

_ALWAYS_OVERRIDDEN = (
    'This file contains defaults for gn values in Chromium which are always '
    'overridden in Brave builds.'
)
_GIVE_A_REASON = (
    'Please provide a reason if you are adding new overrides to this file.'
)

gn_args.config(
    name='brave_defaults',
    defaults_file='brave_defaults.gni',
    description=(
        'This file contains strict overrides for gn values in Chromium which '
        'we always override in Brave builds.\n\n' + _GIVE_A_REASON
    ),
    args={
        'disable_fieldtrial_testing_config': True,
        'root_extra_deps': ['//brave'],
        # Chromium_src overrides do not work with precompiled headers.
        'enable_precompiled_headers': False,
        'enable_pseudolocales': False,
        # Our copy of signature_generator.py doesn't support
        # --ignore_missing_cert.
        'ignore_missing_widevine_signing_cert': False,
        # Brave doesn't report activity via //components/activity_reporter. The
        # value upstream is currently gated on is_chrome_branded, but if that
        # ever changes we want to make sure no reporting to the Google endpoint
        # can happen.
        'report_activity': False,
    },
)

gn_args.config(
    name='blink_platform_defaults',
    defaults_file='blink_platform_defaults.gni',
    description=(
        _ALWAYS_OVERRIDDEN + ' This file in particular is only included in '
        'builds for platforms supporting blink, meaning anything other than '
        'iOS.\n\n' + _GIVE_A_REASON
    ),
    args={
        'proprietary_codecs': True,
        'ffmpeg_branding': 'Chrome',
        'enable_widevine': True,
        # Enable HEVC/H.265 hardware decoding support on all desktop and mobile
        # platforms. Chromium already includes HEVC support for Windows, macOS,
        # and Android, but excludes Linux. Since Brave enables
        # proprietary_codecs, we can enable HEVC platform-wide to allow
        # hardware-accelerated H.265 playback (e.g., via VA-API/NVDEC on Linux).
        'enable_platform_hevc': True,
        'enable_hevc_parser_and_hw_decoder': True,
    },
)

gn_args.config(
    name='branding_defaults',
    defaults_file='branding_defaults.gni',
    args={
        'branding_path_component': 'brave',
        'branding_path_product': 'brave',
    },
)

gn_args.config(
    name='brave_origin_branding_defaults',
    defaults_file='brave_origin/branding_defaults.gni',
    args={
        'branding_path_component': 'brave_origin',
        'branding_path_product': 'brave_origin',
    },
)

gn_args.config(
    name='desktop_defaults',
    defaults_file='desktop_defaults.gni',
    description=(
        _ALWAYS_OVERRIDDEN + ' This file in particular is only included in '
        'builds for non-mobile builds (mac, linux, win)'
    ),
    args={
        'safe_browsing_mode': 1,
    },
)

gn_args.config(
    name='android_defaults',
    defaults_file='android_defaults.gni',
    description=(
        _ALWAYS_OVERRIDDEN + ' This file in particular is only included in '
        'builds for android'
    ),
    args={
        'safe_browsing_mode': 2,
        # Fixes WebRTC IP leak with default option.
        'enable_mdns': True,
        # We want it to be enabled for all configurations.
        'disable_android_lint': False,
        # Compile the extensions core on Android. Runtime use is gated by the
        # BraveAndroidExtensions feature, so this only controls whether the code
        # is in the APK.
        'enable_extensions_core': True,
    },
)

gn_args.config(
    name='ios_defaults',
    defaults_file='ios_defaults.gni',
    description=(
        _ALWAYS_OVERRIDDEN + ' This file in particular is only included in '
        'builds for ios'
    ),
    args={
        'ios_enable_code_signing': False,
        'ios_enable_share_extension': False,
        'ios_enable_credential_provider_extension': True,
        'ios_enable_widget_kit_extension': False,
        # The app currently crashes on launch without disabling these 2 args:
        # https://github.com/brave/brave-browser/issues/49595
        # When this is fixed in Chromium we can remove these 2 arguments.
        'use_partition_alloc_as_malloc': False,
        'enable_backup_ref_ptr_support': False,
        # AllocToken needs the allocator shim's PartitionAlloc-Everywhere
        # support.
        'use_alloc_token': False,
        # Brave iOS relies on static key pinning, which upstream disables for
        # iOS.
        'chrome_key_pinning_supported': True,
        'ios_provider_target': '//brave/ios/browser/providers:brave_providers',
        'ios_locales_pack_extra_source_patterns': [
            '%root_gen_dir%/components/brave_components_strings_'
        ],
        'ios_locales_pack_extra_deps': ['//brave/components/resources:strings'],
        # Configure unit tests to run outside of chromium infra:
        # https://source.chromium.org/chromium/chromium/src/+/main:ios/build/bots/scripts/README.md
        'enable_run_ios_unittests_with_xctest': True,
    },
)

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

# What buildArgs.ts derives for any sanitiser build.
gn_args.config(
    name='sanitiser_defaults',
    configs=[
        'brave_desktop_defaults',
        'ci_desktop_defaults',
        'brave_service_keys',
    ],
    args={
        'is_official_build': False,
        'enable_update_notifications': False,
        'dcheck_always_on': False,
    },
)

gn_args.config(
    name='asan',
    configs=[
        'sanitiser_defaults',
    ],
    args={
        'is_asan': True,
        'is_lsan': True,
        'enable_full_stack_frames_for_profiling': True,
        'v8_enable_verify_heap': True,
    },
)

gn_args.config(
    name='msan',
    configs=[
        'sanitiser_defaults',
    ],
    args={
        'is_msan': True,
        'msan_track_origins': 0,
    },
)

gn_args.config(
    name='ubsan',
    configs=[
        'sanitiser_defaults',
    ],
    args={
        'is_ubsan': True,
        'is_ubsan_vptr': True,
        'is_ubsan_no_recover': True,
    },
)
