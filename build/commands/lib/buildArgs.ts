// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { Config } from './config.js'
import { isCI } from './ciDetect.ts'
import assert from 'node:assert'
import fs from 'node:fs'
import path from 'node:path'

const FORWARD_ENV_CONFIG_VARS_TO_GN_ARGS = [
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

export function getBuildArgs(config: Config) {
  const versionParts = config.braveVersion.split('+')[0]!.split('.')

  let args: Record<string, any> = {
    'import("//brave/build/args/brave_defaults.gni")': null,
    is_asan: config.isAsan(),
    is_lsan: config.isLsan(),
    enable_full_stack_frames_for_profiling: config.isAsan(),
    v8_enable_verify_heap: config.isAsan(),
    is_ubsan: config.is_ubsan,
    is_ubsan_vptr: config.is_ubsan,
    is_ubsan_no_recover: config.is_ubsan,
    is_msan: config.is_msan,
    is_component_build: config.isComponentBuild(),
    is_universal_binary: config.isUniversalBinary,
    target_cpu: config.targetArch,
    is_official_build: config.isOfficialBuild(),
    is_debug: config.isDebug(),
    brave_channel: config.channel,
    brave_version_major: versionParts[0],
    brave_version_minor: versionParts[1],
    brave_version_build: versionParts[2],
    chrome_version_string: config.chromeVersion,
    enable_hangout_services_extension: config.enable_hangout_services_extension,
    enable_cdm_host_verification: config.enableCDMHostVerification(),
    skip_signing: !config.shouldSign(),
    use_remoteexec: config.useRemoteExec,
    use_reclient: config.useReclient,
    use_siso: config.useSiso,
    use_libfuzzer: config.use_libfuzzer,
    enable_update_notifications: config.isOfficialBuild(),
  }

  if (config.targetOS !== 'ios') {
    args['import("//brave/build/args/blink_platform_defaults.gni")'] = null
    if (config.isBraveOriginBranded) {
      args['import("//brave/build/args/brave_origin/branding_defaults.gni")'] =
        null
    } else {
      args['import("//brave/build/args/branding_defaults.gni")'] = null
    }
  } else {
    args['import("//brave/build/args/ios_defaults.gni")'] = null
  }
  if (config.targetOS === 'android') {
    args['import("//brave/build/args/android_defaults.gni")'] = null
  }
  if (config.targetOS !== 'ios' && config.targetOS !== 'android') {
    args['import("//brave/build/args/desktop_defaults.gni")'] = null
  }

  config.forwardEnvConfigVarsToObject(FORWARD_ENV_CONFIG_VARS_TO_GN_ARGS, args)

  if (config.isOfficialBuild()) {
    args.enable_updater = true
  }

  if (args.is_asan || args.is_ubsan || args.is_msan) {
    // Temporarily disabling dcheck_always_on for sanitiser builds, as there
    // are some serious reports coming back. It is necessary first to stabilise
    // these sanitisers with config flag first before using the default.
    args.dcheck_always_on = false
  }

  if (!config.isBraveReleaseBuild()) {
    args.chrome_pgo_phase = 0

    // Don't randomize mojom message ids. When randomization is enabled, all
    // Mojo targets are rebuilt (~23000) on each version bump.
    args.enable_mojom_message_id_scrambling = false

    if (process.platform === 'darwin' && args.is_official_build) {
      // Don't create dSYMs in non-true Release builds. dSYMs should be disabled
      // in order to have relocatable compilation so RBE can share the cache
      // across multiple build directories. Enabled dSYMs enforce absolute
      // paths, which makes RBE cache unusable.
      args.enable_dsyms = false
    }
  }

  if (config.ignorePatchVersionNumber) {
    assert(!config.isBraveReleaseBuild())

    // Allow dummy LASTCHANGE to be set. When the real LASTCHANGE is used, ~2300
    // targets are rebuilt with each version bump.
    args.use_dummy_lastchange = config.useDummyLastchange
  }

  if (config.shouldSign()) {
    if (config.targetOS === 'mac') {
      args.mac_signing_identifier = config.mac_signing_identifier
      args.mac_installer_signing_identifier =
        config.mac_installer_signing_identifier
      args.mac_signing_keychain = config.mac_signing_keychain
      if (config.notarize) {
        args.notarize = true
        args.notary_user = config.notary_user
        args.notary_password = config.notary_password
      }
    } else if (config.targetOS === 'android') {
      args.brave_android_keystore_path = config.braveAndroidKeystorePath
      args.brave_android_keystore_name = config.braveAndroidKeystoreName
      args.brave_android_keystore_password = config.braveAndroidKeystorePassword
      args.brave_android_key_password = config.braveAndroidKeyPassword
      if (config.braveAndroidPkcs11Provider && config.braveAndroidPkcs11Alias) {
        args.brave_android_pkcs11_provider = config.braveAndroidPkcs11Provider
        args.brave_android_pkcs11_alias = config.braveAndroidPkcs11Alias
      }
    }
  }

  if (config.build_omaha) {
    args.build_omaha = config.build_omaha
    args.tag_ap = config.tag_ap
    if (config.tag_installdataindex) {
      args.tag_installdataindex = config.tag_installdataindex
    }
  }

  if (config.last_chrome_installer) {
    args.last_chrome_installer = config.last_chrome_installer
  }

  if (process.platform === 'darwin') {
    args.allow_runtime_configurable_key_storage = true
  }

  if (
    config.isDebug()
    && !config.isComponentBuild()
    && config.targetOS !== 'ios'
    && config.targetOS !== 'android'
  ) {
    args.enable_profiling = true
  }

  if (!config.useSiso) {
    if (config.useRemoteExec) {
      args.reclient_bin_dir = path.join(config.nativeRedirectCCDir)
    } else {
      args.cc_wrapper = path.join(config.nativeRedirectCCDir, 'redirect_cc')
    }
  }

  // Adjust symbol_level to 1 to workaround size restrictions:
  // 1. On Linux x86, ELF32 cannot be > 4GiB.
  // 2. On Linux Static builds, enable symbols (symbol_level is 0 by default in
  //    config). symbol_level = 2 cannot be used because of "relocation
  //    R_X86_64_32 out of range" errors.
  // 3. On Android Release x64/arm64, debug sections exceed 4GiB causing
  //    "relocation R_X86_64_32 out of range" linker errors.
  if (
    (config.targetOS === 'linux'
      && (config.targetArch === 'x86'
        || (!config.isDebug()
          && !config.isComponentBuild()
          && !config.isReleaseBuild())))
    || (config.targetOS === 'android'
      && config.isReleaseBuild()
      && (config.targetArch === 'x64' || config.targetArch === 'arm64'))
  ) {
    args.symbol_level = 1
  }

  if (config.use_clang_coverage) {
    const buildDir = path.relative(config.srcDir, config.outputDir)
    args.use_clang_coverage = true
    args.coverage_instrumentation_input_file = `//${buildDir}/files-to-instrument.txt`
  }

  // For Linux Release builds, upstream doesn't want to use symbol_level = 2
  // unless use_debug_fission is set. However, they don't set it when a
  // cc_wrapper is used. Since we use cc_wrapper we need to set it manually.
  if (config.targetOS === 'linux' && config.isReleaseBuild()) {
    args.use_debug_fission = true
  }

  if (
    config.targetOS === 'mac'
    && fs.existsSync(
      path.join(
        config.srcDir,
        'build',
        'mac_files',
        'xcode_binaries',
        'Contents',
      ),
    )
  ) {
    // always use hermetic xcode for macos when available
    args.use_system_xcode = false
  }

  if (config.targetOS === 'linux') {
    if (config.targetArch !== 'x86') {
      // Include vaapi support
      // TODO: Consider setting use_vaapi_x11 instead of use_vaapi. Also
      // consider enabling it for x86 builds. See
      // https://github.com/brave/brave-browser/issues/1024#issuecomment-1175397914
      args.use_vaapi = true
    }
  }

  // Devtools: Now we patch devtools frontend, so it is useful to see
  // if something goes wrong on CI builds.
  if (config.targetOS !== 'android' && config.targetOS !== 'ios' && isCI) {
    args.devtools_skip_typecheck = false
  }

  if (config.targetOS) {
    args.target_os = config.targetOS
  }

  if (config.targetOS === 'android') {
    args.android_channel = config.channel
    if (!config.isReleaseBuild()) {
      args.android_channel = 'default'
      args.chrome_public_manifest_package = 'com.brave.browser_default'
    } else if (config.channel === '') {
      args.android_channel = 'stable'
      args.chrome_public_manifest_package = 'com.brave.browser'
    } else if (config.channel === 'beta') {
      args.chrome_public_manifest_package = 'com.brave.browser_beta'
    } else if (config.channel === 'dev') {
      args.chrome_public_manifest_package = 'com.brave.browser_dev'
    } else if (config.channel === 'nightly') {
      args.android_channel = 'canary'
      args.chrome_public_manifest_package = 'com.brave.browser_nightly'
    }
    // exclude_unwind_tables is inherited form upstream and is false for any
    // Android build

    args.target_android_output_format =
      config.targetAndroidOutputFormat
      || (config.buildConfig === 'Release' ? 'aab' : 'apk')
    args.android_override_version_name = config.androidOverrideVersionName

    args.brave_android_developer_options_code =
      config.braveAndroidDeveloperOptionsCode
    args.brave_safebrowsing_api_key = config.braveAndroidSafeBrowsingApiKey

    args.android_aab_to_apk = config.androidAabToApk

    if (
      args.target_android_output_format === 'apk'
      && (config.targetArch === 'arm64' || config.targetArch === 'x64')
      && isCI
    ) {
      // We want to have both 32 and 64 bit native libs in arm64/x64 apks for CI
      // Starting from cr136 it is defaulted to false.
      // For local build you can add --gn=enable_android_secondary_abi:true
      // to build both
      args.enable_android_secondary_abi = true
    }

    if (isCI && !config.isOfficialBuild()) {
      // We want Android CI to run Java static analyzer synchronously
      // for non-official (PR) builds.
      // It will be turned off for the official builds
      // (src/build/config/android/config.gni)
      args.android_static_analysis = 'on'
    }

    // Align DCHECKs with Java asserts
    if (args.dcheck_always_on === false) {
      args.enable_java_asserts = false
    }

    // These do not exist on android
    // TODO - recheck
    delete args.enable_hangout_services_extension
  }

  if (config.targetOS === 'ios') {
    if (config.targetEnvironment) {
      args.target_environment = config.targetEnvironment
    }
    if (config.braveIOSMarketingPatchVersion) {
      args.brave_ios_marketing_version_patch =
        config.braveIOSMarketingPatchVersion
    }
    // Component builds are not supported for iOS:
    // https://chromium.googlesource.com/chromium/src/+/master/docs/component_build.md
    args.is_component_build = false

    if (!args.is_official_build) {
      // When building locally iOS needs dSYMs in order for Xcode to map source
      // files correctly since we are using a framework build
      args.enable_dsyms = true
      if (args.use_remoteexec) {
        // RBE expects relative paths in dSYMs
        args.strip_absolute_paths_from_debug_symbols = true
      }
    }

    args.brave_ios_developer_options_code = config.braveIOSDeveloperOptionsCode

    delete args.safebrowsing_api_endpoint
    delete args.enable_hangout_services_extension
    delete args.brave_google_api_endpoint
    delete args.brave_google_api_key
    delete args.brave_stats_updater_url
    delete args.bitflyer_production_client_id
    delete args.bitflyer_production_client_secret
    delete args.bitflyer_production_fee_address
    delete args.bitflyer_production_url
    delete args.bitflyer_sandbox_client_id
    delete args.bitflyer_sandbox_client_secret
    delete args.bitflyer_sandbox_fee_address
    delete args.bitflyer_sandbox_url
    delete args.gemini_production_api_url
    delete args.gemini_production_fee_address
    delete args.gemini_production_oauth_url
    delete args.gemini_sandbox_api_url
    delete args.gemini_sandbox_fee_address
    delete args.gemini_sandbox_oauth_url
    delete args.uphold_production_api_url
    delete args.uphold_production_fee_address
    delete args.uphold_production_oauth_url
    delete args.uphold_sandbox_api_url
    delete args.uphold_sandbox_fee_address
    delete args.uphold_sandbox_oauth_url
    delete args.zebpay_production_api_url
    delete args.zebpay_production_oauth_url
    delete args.zebpay_sandbox_api_url
    delete args.zebpay_sandbox_oauth_url
    delete args.use_blink_v8_binding_new_idl_interface
    delete args.v8_enable_verify_heap
    delete args.service_key_stt
  }

  args = Object.assign(args, config.extraGnArgs)
  return args
}
