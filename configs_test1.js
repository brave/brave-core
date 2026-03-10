// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const path = require('path')
const fs = require('fs')
const { gnConfig, gnRule } = require('../commands/lib/gnConfigRegistry')
const EnvConfig = require('../commands/lib/envConfig')

const braveCoreDir = path.resolve(__dirname, '..', '..')
const srcDir = path.resolve(braveCoreDir, '..')
const envConfig = new EnvConfig(braveCoreDir)

const getEnvConfig = (keyPath, defaultValue = undefined) => {
  return envConfig.get(keyPath, defaultValue)
}

const isBraveReleaseBuild = () => {
  const val = getEnvConfig(['is_brave_release_build'])
  return val === 1
}

const isBraveOriginBranded = () => {
  const val = getEnvConfig(['is_brave_origin_branded'])
  return val === true
}

const getBraveVersion = (ignorePatch) => {
  const version = envConfig.getPackageVersion()
  if (!ignorePatch) return version
  const parts = version.split('.')
  parts[2] = '0'
  return parts.join('.')
}

const nativeRedirectCCDir = path.join(srcDir, 'out', 'redirect_cc')

// ── Brave Defaults ──────────────────────────────────

gnConfig('brave_defaults', {
  args: {
    disable_fieldtrial_testing_config: true,
    root_extra_deps: ['//brave'],
    translate_genders: false,
    enable_precompiled_headers: false,
    enable_pseudolocales: false,
    ignore_missing_widevine_signing_cert: false,
  },
  isDefault: () => true,
})

gnConfig('branding_brave', {
  group: 'branding',
  args: {
    branding_path_component: 'brave',
    branding_path_product: 'brave',
  },
  isDefault: () => !isBraveOriginBranded(),
})

gnConfig('branding', {
  group: 'branding',
  args: () => {
    return {
      ...(isBraveOriginBranded() && {
        branding_path_component: 'brave_origin',
        branding_path_product: 'brave_origin',
      }),
      ...(!isBraveOriginBranded() && {
        branding_path_component: 'brave',
        branding_path_product: 'brave',
      }),
    }
  },
  isDefault: () => true,
})

gnConfig('blink_platform_defaults', {
  args: {
    proprietary_codecs: true,
    ffmpeg_branding: 'Chrome',
    enable_glic: false,
    enable_widevine: true,
  },
})

gnConfig('desktop_defaults', {
  args: {
    safe_browsing_mode: 1,
  },
})

// ── Target OS ─────────────────────────────────────────────

gnConfig('android', {
  group: 'target_os',
  configs: ['brave_defaults', 'blink_platform_defaults'],
  args: ({ has }) => {
    const args = {
      target_os: 'android',
      enable_hangout_services_extension: false,
      safe_browsing_mode: 2,
      enable_mdns: true,
      disable_android_lint: false,
    }
    // Channel mapping: the GN android_channel differs from brave_channel.
    const channel =
      has('release') || has('brave_release')
        ? getEnvConfig(['brave_channel']) || ''
        : 'default'
    if (!has('release') && !has('brave_release')) {
      args.android_channel = 'default'
      args.chrome_public_manifest_package = 'com.brave.browser_default'
    } else if (channel === '' || channel === 'release') {
      args.android_channel = 'stable'
      args.chrome_public_manifest_package = 'com.brave.browser'
    } else if (channel === 'beta') {
      args.android_channel = 'beta'
      args.chrome_public_manifest_package = 'com.brave.browser_beta'
    } else if (channel === 'dev') {
      args.android_channel = 'dev'
      args.chrome_public_manifest_package = 'com.brave.browser_dev'
    } else if (channel === 'nightly') {
      args.android_channel = 'canary'
      args.chrome_public_manifest_package = 'com.brave.browser_nightly'
    }

    const isRelease = has('release') || has('brave_release')
    args.target_android_output_format = isRelease ? 'aab' : 'apk'
    args.android_aab_to_apk = false

    if (has('ci')) {
      const isArm64 = has('arm64')
      const isX64 = has('x64')
      if (args.target_android_output_format === 'apk' && (isArm64 || isX64)) {
        args.enable_android_secondary_abi = true
      }

      if (!has('brave_release')) {
        args.android_static_analysis = 'on'
      }
    }

    if (!has('brave_release')) {
      args.enable_java_asserts = false
    }
  },
})

gnConfig('ios', {
  group: 'target_os',
  configs: ['brave_defaults', 'ios_defaults'],
  args: {
    target_os: 'ios',
    is_component_build: false,
    ios_enable_code_signing: false,
    ios_enable_share_extension: false,
    ios_enable_credential_provider_extension: true,
    ios_enable_widget_kit_extension: false,
    use_partition_alloc_as_malloc: false,
    enable_backup_ref_ptr_support: false,
    ios_provider_target: '//brave/ios/browser/providers:brave_providers',
    ios_locales_pack_extra_source_patterns: [
      '%root_gen_dir%/components/brave_components_strings_',
    ],
    ios_locales_pack_extra_deps: ['//brave/components/resources:strings'],
    enable_run_ios_unittests_with_xctest: true,
  },
  apply: ({ has, args }) => {
    const targetEnv = getEnvConfig(['target_environment'])
    if (targetEnv) {
      args.target_environment = targetEnv
    }

    const marketingPatch =
      getEnvConfig(['brave_ios_marketing_version_patch']) || ''
    if (marketingPatch) {
      args.brave_ios_marketing_version_patch = marketingPatch
    }

    if (!args.is_official_build) {
      args.enable_dsyms = true
      if (args.use_remoteexec) {
        args.strip_absolute_paths_from_debug_symbols = true
      }
    }

    args.brave_ios_developer_options_code = getEnvConfig([
      'brave_ios_developer_options_code',
    ])

    // Many GN args from env forwarding are not applicable to iOS.
    const iosDeletedArgs = [
      'safebrowsing_api_endpoint',
      'enable_hangout_services_extension',
      'brave_google_api_endpoint',
      'brave_google_api_key',
      'brave_stats_updater_url',
      'bitflyer_production_client_id',
      'bitflyer_production_client_secret',
      'bitflyer_production_fee_address',
      'bitflyer_production_url',
      'bitflyer_sandbox_client_id',
      'bitflyer_sandbox_client_secret',
      'bitflyer_sandbox_fee_address',
      'bitflyer_sandbox_url',
      'gemini_production_api_url',
      'gemini_production_fee_address',
      'gemini_production_oauth_url',
      'gemini_sandbox_api_url',
      'gemini_sandbox_fee_address',
      'gemini_sandbox_oauth_url',
      'uphold_production_api_url',
      'uphold_production_fee_address',
      'uphold_production_oauth_url',
      'uphold_sandbox_api_url',
      'uphold_sandbox_fee_address',
      'uphold_sandbox_oauth_url',
      'zebpay_production_api_url',
      'zebpay_production_oauth_url',
      'zebpay_sandbox_api_url',
      'zebpay_sandbox_oauth_url',
      'use_blink_v8_binding_new_idl_interface',
      'v8_enable_verify_heap',
      'service_key_stt',
    ]
    for (const key of iosDeletedArgs) {
      delete args[key]
    }
  },
})

gnConfig('linux', {
  group: 'target_os',
  auto: () => process.platform === 'linux',
  configs: ['brave_defaults', 'blink_platform_defaults', 'desktop_defaults'],
  args: {
    target_os: 'linux',
  },
  apply: ({ has, args }) => {
    const targetCpu = args.target_cpu
    if (targetCpu !== 'x86') {
      args.use_vaapi = true
    }

    // Workaround ELF32 4GiB limit on x86, and enable symbols in static
    // (non-debug, non-component, non-release) builds.
    if (
      targetCpu === 'x86'
      || (!has('debug') && !has('release') && !has('brave_release'))
    ) {
      args.symbol_level = 1
    }

    if (has('release') || has('brave_release')) {
      args.use_debug_fission = true
    }
  },
})

gnConfig('mac', {
  group: 'target_os',
  auto: () => process.platform === 'darwin',
  configs: ['brave_defaults', 'blink_platform_defaults', 'desktop_defaults'],
  args: {
    target_os: 'mac',
    allow_runtime_configurable_key_storage: true,
  },
  apply: ({ has, args }) => {
    const hermeticXcodePath = path.join(
      srcDir,
      'build',
      'mac_files',
      'xcode_binaries',
      'Contents',
    )
    if (fs.existsSync(hermeticXcodePath)) {
      args.use_system_xcode = false
    }
  },
})

gnConfig('win', {
  group: 'target_os',
  auto: () => process.platform === 'win32',
  configs: ['brave_defaults', 'blink_platform_defaults', 'desktop_defaults'],
  args: {
    target_os: 'win',
  },
})

// ── Target CPU ────────────────────────────────────────────

gnConfig('x64', {
  group: 'target_cpu',
  auto: () => process.arch === 'x64',
  args: { target_cpu: 'x64' },
})

gnConfig('arm64', {
  group: 'target_cpu',
  auto: () => process.arch === 'arm64',
  args: { target_cpu: 'arm64' },
})

gnConfig('x86', {
  group: 'target_cpu',
  args: { target_cpu: 'x86' },
})

gnConfig('arm', {
  group: 'target_cpu',
  args: { target_cpu: 'arm' },
})

// ── Build Type ────────────────────────────────────────────

gnConfig('debug', {
  group: 'build_type',
  args: {
    is_debug: true,
    is_component_build: true,
    symbol_level: 2,
    is_official_build: false,
    enable_update_notifications: false,
  },
  apply: ({ has, args }) => {
    // Enable profiling for non-component desktop debug builds.
    if (!args.is_component_build && !has('ios') && !has('android')) {
      args.enable_profiling = true
    }
  },
})

gnConfig('release', {
  group: 'build_type',
  args: {
    is_debug: false,
    is_official_build: true,
    enable_update_notifications: true,
    enable_updater: true,
    chrome_pgo_phase: 0,
    enable_mojom_message_id_scrambling: false,
  },
  apply: ({ has, args }) => {
    if (has('asan') || has('ubsan') || has('msan')) {
      // Sanitizer release builds are not truly "official".
      args.is_official_build = false
      args.enable_update_notifications = false
      delete args.enable_updater
    }

    // Non-brave-release builds disable dSYMs on macOS for RBE cache sharing.
    if (!has('brave_release') && has('mac')) {
      args.enable_dsyms = false
    }
  },
})

gnConfig('brave_release', {
  group: 'build_type',
  args: {
    is_debug: false,
    is_official_build: true,
    enable_update_notifications: true,
    enable_updater: true,
  },
})

// ── Sanitizers ────────────────────────────────────────────

gnConfig('no_component_build', {
  args: {
    is_component_build: false,
  },
})

gnConfig('asan', {
  configs: ['no_component_build'],
  args: {
    is_asan: true,
    enable_full_stack_frames_for_profiling: true,
    v8_enable_verify_heap: true,
    dcheck_always_on: false,
  },
  apply: ({ has, args }) => {
    // LSAN only works with ASAN on x86_64 Linux.
    if (has('linux')) {
      args.is_lsan = true
    } else {
      args.is_lsan = false
    }
  },
})

gnConfig('ubsan', {
  configs: ['no_component_build'],
  args: {
    is_ubsan: true,
    is_ubsan_vptr: true,
    is_ubsan_no_recover: true,
    dcheck_always_on: false,
  },
})

gnConfig('msan', {
  configs: ['no_component_build'],
  args: {
    is_msan: true,
    dcheck_always_on: false,
  },
})

gnConfig('libfuzzer', {
  args: {
    use_libfuzzer: true,
  },
})

// ── Environment / CI ──────────────────────────────────────

gnConfig('ci', {
  group: 'environment',
  auto: () => !!process.env.BUILD_ID || !!process.env.TEAMCITY_VERSION,
  args: {
    use_siso: true,
  },
  apply: ({ has, args }) => {
    // Enable devtools typecheck on CI desktop builds.
    if (!has('android') && !has('ios')) {
      args.devtools_skip_typecheck = false
    }
  },
})

// ── Non-Brave-Release Optimizations ───────────────────────
//
// Applied automatically when the build is not a true brave release.
// Reduces rebuild churn by disabling PGO and mojom scrambling.

gnConfig('non_brave_release_opts', {
  group: 'release_tier',
  auto: () => !isBraveReleaseBuild(),
  args: {
    chrome_pgo_phase: 0,
    enable_mojom_message_id_scrambling: false,
  },
  apply: ({ has, args }) => {
    const ignorePatch =
      !isBraveReleaseBuild()
      && getEnvConfig(
        ['ignore_patch_version_number'],
        !(!!process.env.BUILD_ID || !!process.env.TEAMCITY_VERSION),
      )
    if (ignorePatch) {
      args.use_dummy_lastchange = getEnvConfig(['use_dummy_lastchange'], true)
    }
  },
})

gnConfig('brave_release_tier', {
  group: 'release_tier',
  auto: () => isBraveReleaseBuild(),
  args: {},
})

// ── Toolchain / Build Backend ─────────────────────────────

gnConfig('siso', {
  group: 'build_backend',
  auto: () => getEnvConfig(['use_siso'], true),
  args: {
    use_siso: true,
    use_remoteexec: false,
    use_reclient: false,
  },
  apply: ({ has, args }) => {
    const useRemoteExec = getEnvConfig(['use_remoteexec'], false)
    if (useRemoteExec) {
      args.use_remoteexec = true
    }
  },
})

gnConfig('reclient', {
  group: 'build_backend',
  args: {
    use_siso: false,
    use_reclient: true,
    use_remoteexec: true,
  },
  apply: ({ has, args }) => {
    args.reclient_bin_dir = nativeRedirectCCDir
  },
})

gnConfig('local_build', {
  group: 'build_backend',
  auto: () => !getEnvConfig(['use_siso'], true),
  args: {
    use_siso: false,
    use_remoteexec: false,
    use_reclient: false,
  },
  apply: ({ has, args }) => {
    args.cc_wrapper = path.join(nativeRedirectCCDir, 'redirect_cc')
  },
})

// ── Signing ───────────────────────────────────────────────
//
// Signing is opt-in. When included, the apply callback detects credentials
// from the environment and sets platform-appropriate signing args.

gnConfig('signing', {
  args: {
    skip_signing: false,
  },
  apply: ({ has, args }) => {
    if (has('mac')) {
      const ident = getEnvConfig(['mac_signing_identifier'])
      if (ident) {
        args.mac_signing_identifier = ident
        args.mac_installer_signing_identifier =
          getEnvConfig(['mac_installer_signing_identifier']) || ''
        args.mac_signing_keychain =
          getEnvConfig(['mac_signing_keychain']) || 'login'

        const notaryUser = getEnvConfig(['notary_user'])
        const notaryPassword = getEnvConfig(['notary_password'])
        if (notaryUser && notaryPassword) {
          args.notarize = true
          args.notary_user = notaryUser
          args.notary_password = notaryPassword
        }
      }
    } else if (has('android')) {
      const keystorePath = getEnvConfig(['brave_android_keystore_path'])
      if (keystorePath) {
        args.brave_android_keystore_path = keystorePath
        args.brave_android_keystore_name = getEnvConfig([
          'brave_android_keystore_name',
        ])
        args.brave_android_keystore_password = getEnvConfig([
          'brave_android_keystore_password',
        ])
        args.brave_android_key_password = getEnvConfig([
          'brave_android_key_password',
        ])

        const pkcs11Provider = getEnvConfig(['brave_android_pkcs11_provider'])
        const pkcs11Alias = getEnvConfig(['brave_android_pkcs11_alias'])
        if (pkcs11Provider && pkcs11Alias) {
          args.brave_android_pkcs11_provider = pkcs11Provider
          args.brave_android_pkcs11_alias = pkcs11Alias
        }
      }
    }
  },
})

gnConfig('no_signing', {
  args: {
    skip_signing: true,
  },
})

// ── CDM Host Verification (Widevine) ─────────────────────

gnConfig('cdm_host_verification', {
  args: {},
  apply: ({ has, args }) => {
    const signCert = process.env.SIGN_WIDEVINE_CERT || ''
    const signKey = process.env.SIGN_WIDEVINE_KEY || ''
    const signPasswd = process.env.SIGN_WIDEVINE_PASSPHRASE || ''
    const sigGen = path.join(
      srcDir,
      'third_party',
      'widevine',
      'scripts',
      'signature_generator.py',
    )

    const enable =
      (has('release') || has('brave_release'))
      && !has('linux')
      && signCert !== ''
      && signKey !== ''
      && signPasswd !== ''
      && fs.existsSync(sigGen)

    args.enable_cdm_host_verification = enable
  },
})

// ── Versioning ────────────────────────────────────────────
//
// Always-present config that computes version args from package.json.

gnConfig('brave_version', {
  args: {},
  apply: ({ has, args }) => {
    const isCI = has('ci')
    const ignorePatch =
      !isBraveReleaseBuild()
      && getEnvConfig(['ignore_patch_version_number'], !isCI)
    const version = getBraveVersion(ignorePatch)
    const versionParts = version.split('+')[0].split('.')

    args.brave_version_major = versionParts[0]
    args.brave_version_minor = versionParts[1]
    args.brave_version_build = versionParts[2]

    const chromeVersion =
      getEnvConfig(['projects', 'chrome', 'revision'])
      || getEnvConfig(['projects', 'chrome', 'tag'])
      || getEnvConfig(['projects', 'chrome', 'branch'])
    args.chrome_version_string = chromeVersion

    if (has('android')) {
      args.android_override_version_name = version
    }
  },
})

// ── Channel ───────────────────────────────────────────────

gnConfig('channel_development', {
  group: 'channel',
  auto: () => !isBraveReleaseBuild(),
  args: {
    brave_channel: 'development',
  },
})

gnConfig('channel_release', {
  group: 'channel',
  args: {
    brave_channel: '',
  },
})

gnConfig('channel_beta', {
  group: 'channel',
  args: {
    brave_channel: 'beta',
  },
})

gnConfig('channel_dev', {
  group: 'channel',
  args: {
    brave_channel: 'dev',
  },
})

gnConfig('channel_nightly', {
  group: 'channel',
  args: {
    brave_channel: 'nightly',
  },
})

// ── Environment-Forwarded Args ────────────────────────────
//
// Replaces the forwardEnvArgsToGn loop. Each key is read from envConfig
// and set as a GN arg if defined. iOS-specific filtering happens in the
// ios config's apply callback (deletes inapplicable keys).

gnConfig('env_forwarded_args', {
  args: {},
  apply: ({ has, args }) => {
    const forwardKeys = [
      'bitflyer_production_client_id',
      'bitflyer_production_client_secret',
      'bitflyer_production_fee_address',
      'bitflyer_production_url',
      'bitflyer_sandbox_client_id',
      'bitflyer_sandbox_client_secret',
      'bitflyer_sandbox_fee_address',
      'bitflyer_sandbox_url',
      'brave_android_developer_options_code',
      'brave_google_api_endpoint',
      'brave_google_api_key',
      'brave_safebrowsing_api_key',
      'brave_services_dev_domain',
      'brave_services_key_id',
      'brave_services_production_domain',
      'brave_services_staging_domain',
      'brave_stats_api_key',
      'brave_stats_updater_url',
      'brave_sync_endpoint',
      'brave_variations_server_url',
      'concurrent_links',
      'dcheck_always_on',
      'enable_updater',
      'gemini_production_api_url',
      'gemini_production_fee_address',
      'gemini_production_oauth_url',
      'gemini_sandbox_api_url',
      'gemini_sandbox_fee_address',
      'gemini_sandbox_oauth_url',
      'google_default_client_id',
      'google_default_client_secret',
      'msan_track_origins',
      'rewards_grant_dev_endpoint',
      'rewards_grant_prod_endpoint',
      'rewards_grant_staging_endpoint',
      'safebrowsing_api_endpoint',
      'service_key_aichat',
      'service_key_stt',
      'sparkle_dsa_private_key_file',
      'sparkle_eddsa_private_key',
      'sparkle_eddsa_public_key',
      'updater_dev_endpoint',
      'updater_prod_endpoint',
      'uphold_production_api_url',
      'uphold_production_fee_address',
      'uphold_production_oauth_url',
      'uphold_sandbox_api_url',
      'uphold_sandbox_fee_address',
      'uphold_sandbox_oauth_url',
      'use_prebuilt_omaha4',
      'webcompat_report_api_endpoint',
      'zebpay_production_api_url',
      'zebpay_production_oauth_url',
      'zebpay_sandbox_api_url',
      'zebpay_sandbox_oauth_url',
      'use_clang_coverage',
      'coverage_instrumentation_input_file',
      'is_brave_origin_branded',
    ]

    for (const key of forwardKeys) {
      const val = getEnvConfig([key])
      if (val !== undefined) {
        args[key] = val
      }
    }
  },
})

// ── Omaha (Windows installer) ─────────────────────────────

gnConfig('omaha', {
  args: {},
  apply: ({ has, args }) => {
    args.build_omaha = true
    const tagAp = getEnvConfig(['tag_ap'])
    if (tagAp) {
      args.tag_ap = tagAp
    }
    const tagIdx = getEnvConfig(['tag_installdataindex'])
    if (tagIdx) {
      args.tag_installdataindex = tagIdx
    }
  },
})

// ── Last Chrome Installer (Windows delta updates) ─────────

gnConfig('last_chrome_installer', {
  args: {},
  apply: ({ has, args }) => {
    const installer = getEnvConfig(['last_chrome_installer'])
    if (installer) {
      args.last_chrome_installer = installer
    }
  },
})

// ── Universal Binary (macOS) ──────────────────────────────

gnConfig('universal_binary', {
  args: {
    is_universal_binary: true,
    target_cpu: 'arm64',
  },
})

// ── Clang Coverage ────────────────────────────────────────

gnConfig('clang_coverage', {
  args: {
    use_clang_coverage: true,
  },
  apply: ({ has, args }) => {
    // coverage_instrumentation_input_file needs the build dir, which is
    // only known at generation time. The resolver should supply outputDir
    // via context; for now we leave a placeholder pattern.
    // In practice the CLI wrapper fills this in.
  },
})

// ── Branding Variant ──────────────────────────────────────

gnConfig('brave_origin_branded', {
  configs: ['branding_brave_origin'],
  args: {},
})

// ── Compound Presets ──────────────────────────────────────

gnConfig('ci_android_arm64', {
  configs: [
    'android',
    'arm64',
    'brave_release',
    'ci',
    'signing',
    'brave_version',
    'env_forwarded_args',
    'cdm_host_verification',
  ],
})

gnConfig('ci_android_x64', {
  configs: [
    'android',
    'x64',
    'brave_release',
    'ci',
    'signing',
    'brave_version',
    'env_forwarded_args',
    'cdm_host_verification',
  ],
})

gnConfig('ci_linux_x64', {
  configs: [
    'linux',
    'x64',
    'brave_release',
    'ci',
    'signing',
    'brave_version',
    'env_forwarded_args',
    'cdm_host_verification',
  ],
})

gnConfig('ci_mac_arm64', {
  configs: [
    'mac',
    'arm64',
    'brave_release',
    'ci',
    'signing',
    'brave_version',
    'env_forwarded_args',
    'cdm_host_verification',
  ],
})

gnConfig('ci_mac_x64', {
  configs: [
    'mac',
    'x64',
    'brave_release',
    'ci',
    'signing',
    'brave_version',
    'env_forwarded_args',
    'cdm_host_verification',
  ],
})

gnConfig('ci_win_x64', {
  configs: [
    'win',
    'x64',
    'brave_release',
    'ci',
    'signing',
    'brave_version',
    'env_forwarded_args',
    'cdm_host_verification',
  ],
})

gnConfig('ci_ios_arm64', {
  configs: [
    'ios',
    'arm64',
    'brave_release',
    'ci',
    'brave_version',
    'env_forwarded_args',
  ],
})

gnConfig('dev_android', {
  configs: [
    'android',
    'arm64',
    'debug',
    'no_signing',
    'brave_version',
    'env_forwarded_args',
    'cdm_host_verification',
  ],
})

gnConfig('dev_desktop', {
  configs: [
    'debug',
    'no_signing',
    'brave_version',
    'env_forwarded_args',
    'cdm_host_verification',
  ],
})

gnConfig('dev_ios', {
  configs: ['ios', 'arm64', 'debug', 'brave_version', 'env_forwarded_args'],
})

// ── Validation Rules ──────────────────────────────────────

gnRule('android requires target_cpu', (args) => {
  if (args.target_os === 'android' && !args.target_cpu) {
    throw new Error('Android builds must specify target_cpu')
  }
})

gnRule('official builds are not debug', (args) => {
  if (args.is_official_build && args.is_debug) {
    throw new Error('Official builds must not have is_debug = true')
  }
})

gnRule('ios does not support component builds', (args) => {
  if (args.target_os === 'ios' && args.is_component_build) {
    throw new Error('iOS does not support component builds')
  }
})

gnRule('sanitizers disable official build', (args) => {
  if (
    (args.is_asan || args.is_ubsan || args.is_msan)
    && args.is_official_build
  ) {
    throw new Error(
      'Sanitizer builds (asan/ubsan/msan) must not be official builds',
    )
  }
})
