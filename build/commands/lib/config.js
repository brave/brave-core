// Copyright (c) 2016 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const path = require('path')
const fs = require('fs')
const assert = require('assert')
const dotenvPopulateWithIncludes = require('./dotenvPopulateWithIncludes')
const Log = require('./logging')

let envConfig = null

let dirName = __dirname
// Use fs.realpathSync to normalize the path(__dirname could be c:\.. or C:\..).
if (process.platform === 'win32') {
  dirName = fs.realpathSync.native(dirName)
}
const rootDir = path.resolve(dirName, '..', '..', '..', '..', '..')
const braveCoreDir = path.join(rootDir, 'src', 'brave')

if (rootDir.includes(' ')) {
  Log.error(`Root directory contains spaces, this is not supported: ${rootDir}`)
  process.exit(1)
}

const packageConfig = function (key, sourceDir = braveCoreDir) {
  let packages = { config: {} }
  const configAbsolutePath = path.join(sourceDir, 'package.json')
  if (fs.existsSync(configAbsolutePath)) {
    packages = require(path.relative(__dirname, configAbsolutePath))
  }

  // packages.config should include version string.
  let obj = Object.assign({}, packages.config, { version: packages.version })
  for (let i = 0, len = key.length; i < len; i++) {
    if (!obj) {
      return obj
    }
    obj = obj[key[i]]
  }
  return obj
}

const readArgsGn = (srcDir, outputDir) => {
  const util = require('./util')
  const gnHelpersPath = path.join(srcDir, 'build', 'gn_helpers.py')

  const script = `
import sys
import os
sys.path.insert(0, '${path.dirname(gnHelpersPath)}')
import gn_helpers
result = gn_helpers.ReadArgsGN('${outputDir}')
import json
print(json.dumps(result))
`

  const result = util.run('python3', ['-'], {
    skipLogging: true,
    input: script,
    encoding: 'utf8',
  })

  return JSON.parse(result.stdout.toString().trim())
}

const getEnvConfig = (key, defaultValue = undefined) => {
  if (!envConfig) {
    envConfig = {}

    // Parse src/brave/.env with all included env files.
    let envConfigPath = path.join(braveCoreDir, '.env')
    if (fs.existsSync(envConfigPath)) {
      dotenvPopulateWithIncludes(envConfig, envConfigPath)
    } else {
      // The .env file is used by `gn gen`. Create it if it doesn't exist.
      const defaultEnvConfigContent =
        '# This is a placeholder .env config file for the build system.\n'
        + '# See for details: https://github.com/brave/brave-browser/wiki/Build-configuration\n'
      fs.writeFileSync(envConfigPath, defaultEnvConfigContent)
    }

    // Convert 'true' and 'false' strings into booleans.
    for (const [key, value] of Object.entries(envConfig)) {
      try {
        envConfig[key] = JSON.parse(value)
      } catch (e) {
        envConfig[key] = value
      }
    }
  }

  const envConfigValue = envConfig[key.join('_')]
  if (envConfigValue !== undefined) {
    return envConfigValue
  }

  const packageConfigValue = packageConfig(key)
  if (packageConfigValue !== undefined) {
    return packageConfigValue
  }

  return defaultValue
}

const getDepotToolsDir = (rootDir) => {
  let depotToolsDir = getEnvConfig(['projects', 'depot_tools', 'dir'])
  if (!path.isAbsolute(depotToolsDir)) {
    depotToolsDir = path.join(rootDir, depotToolsDir)
  }
  return path.normalize(depotToolsDir)
}

const parseExtraInputs = (inputs, accumulator, callback) => {
  for (let input of inputs) {
    let separatorIndex = input.indexOf(':')
    if (separatorIndex < 0) {
      separatorIndex = input.length
    }

    const key = input.substring(0, separatorIndex)
    const value = input.substring(separatorIndex + 1)
    callback(accumulator, key, value)
  }
}

const getBraveVersion = (ignorePatchVersionNumber) => {
  const braveVersion = packageConfig(['version'])
  if (!ignorePatchVersionNumber) {
    return braveVersion
  }

  const braveVersionParts = braveVersion.split('.')
  assert(braveVersionParts.length === 3)
  braveVersionParts[2] = '0'
  return braveVersionParts.join('.')
}

const getHostOS = () => {
  switch (process.platform) {
    case 'darwin':
      return 'mac'
    case 'linux':
      return 'linux'
    case 'win32':
      return 'win'
    default:
      throw new Error(`Unsupported process.platform: ${process.platform}`)
  }
}

const Config = function () {
  this.isTeamcity = process.env.TEAMCITY_VERSION !== undefined
  this.isCI = process.env.BUILD_ID !== undefined || this.isTeamcity
  this.internalDepsUrl =
    'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.on.aws'
  this.defaultBuildConfig =
    getEnvConfig(['default_build_config']) || 'Component'
  this.buildConfig = this.defaultBuildConfig
  this.buildTargets = ['brave']
  this.rootDir = rootDir
  this.isUniversalBinary = false
  this.isChromium = false
  this.scriptDir = path.join(this.rootDir, 'scripts')
  this.srcDir = path.join(this.rootDir, 'src')
  this.chromeVersion = this.getProjectVersion('chrome')
  this.chromiumRepo = getEnvConfig(['projects', 'chrome', 'repository', 'url'])
  this.braveCoreDir = braveCoreDir
  this.buildToolsDir = path.join(this.srcDir, 'build')
  this.resourcesDir = path.join(this.rootDir, 'resources')
  this.depotToolsDir = getDepotToolsDir(this.braveCoreDir)
  this.depotToolsRepo = getEnvConfig([
    'projects',
    'depot_tools',
    'repository',
    'url',
  ])
  this.defaultGClientFile = path.join(this.rootDir, '.gclient')
  this.gClientFile = process.env.BRAVE_GCLIENT_FILE || this.defaultGClientFile
  this.gClientVerbose = getEnvConfig(['gclient_verbose']) || false
  this.hostOS = getHostOS()
  this.targetArch = getEnvConfig(['target_arch']) || process.arch
  this.targetOS = getEnvConfig(['target_os'])
  this.targetEnvironment = getEnvConfig(['target_environment'])
  this.gypTargetArch = 'x64'
  this.targetAndroidBase = 'mono'
  this.ignorePatchVersionNumber =
    !this.isBraveReleaseBuild()
    && getEnvConfig(['ignore_patch_version_number'], !this.isCI)
  this.braveVersion = getBraveVersion(this.ignorePatchVersionNumber)
  this.braveIOSMarketingPatchVersion =
    getEnvConfig(['brave_ios_marketing_version_patch']) || ''
  this.androidOverrideVersionName = this.braveVersion
  this.releaseTag = this.braveVersion.split('+')[0]
  this.mac_signing_identifier = getEnvConfig(['mac_signing_identifier'])
  this.mac_installer_signing_identifier =
    getEnvConfig(['mac_installer_signing_identifier']) || ''
  this.mac_signing_keychain = getEnvConfig(['mac_signing_keychain']) || 'login'
  this.notary_user = getEnvConfig(['notary_user'])
  this.notary_password = getEnvConfig(['notary_password'])
  this.channel = 'development'
  this.git_cache_path = getEnvConfig(['git_cache_path'])
  this.rbeService = getEnvConfig(['rbe_service']) || ''
  this.rbeTlsClientAuthCert = getEnvConfig(['rbe_tls_client_auth_cert']) || ''
  this.rbeTlsClientAuthKey = getEnvConfig(['rbe_tls_client_auth_key']) || ''
  this.realRewrapperDir =
    process.env.RBE_DIR || path.join(this.srcDir, 'buildtools', 'reclient')
  this.ignore_compile_failure = false
  this.enable_hangout_services_extension = false
  this.sign_widevine_cert = process.env.SIGN_WIDEVINE_CERT || ''
  this.sign_widevine_key = process.env.SIGN_WIDEVINE_KEY || ''
  this.sign_widevine_passwd = process.env.SIGN_WIDEVINE_PASSPHRASE || ''
  this.signature_generator =
    path.join(
      this.srcDir,
      'third_party',
      'widevine',
      'scripts',
      'signature_generator.py',
    ) || ''
  this.extraGnArgs = {}
  this.extraGnGenOpts = getEnvConfig(['brave_extra_gn_gen_opts']) || ''
  this.extraNinjaOpts = []
  this.sisoJobsLimit = undefined
  this.braveAndroidSafeBrowsingApiKey = getEnvConfig([
    'brave_safebrowsing_api_key',
  ])
  this.braveAndroidDeveloperOptionsCode = getEnvConfig([
    'brave_android_developer_options_code',
  ])
  this.braveAndroidKeystorePath = getEnvConfig(['brave_android_keystore_path'])
  this.braveAndroidKeystoreName = getEnvConfig(['brave_android_keystore_name'])
  this.braveAndroidKeystorePassword = getEnvConfig([
    'brave_android_keystore_password',
  ])
  this.braveAndroidKeyPassword = getEnvConfig(['brave_android_key_password'])
  this.braveAndroidPkcs11Provider = ''
  this.braveAndroidPkcs11Alias = ''
  this.nativeRedirectCCDir = path.join(this.srcDir, 'out', 'redirect_cc')
  this.useRemoteExec = getEnvConfig(['use_remoteexec']) || false
  this.offline = getEnvConfig(['offline']) || false
  this.use_libfuzzer = false
  this.androidAabToApk = false
  this.useBraveHermeticToolchain = this.rbeService.includes('.brave.com:')
  this.braveIOSDeveloperOptionsCode = getEnvConfig([
    'brave_ios_developer_options_code',
  ])
  this.skip_download_rust_toolchain_aux =
    getEnvConfig(['skip_download_rust_toolchain_aux']) || false
  this.is_msan = getEnvConfig(['is_msan'])
  this.is_ubsan = getEnvConfig(['is_ubsan'])
  this.use_no_gn_gen = getEnvConfig(['use_no_gn_gen'])

  this.forwardEnvArgsToGn = [
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
    'brave_infura_project_id',
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
    'gemini_production_client_id',
    'gemini_production_client_secret',
    'gemini_production_fee_address',
    'gemini_production_oauth_url',
    'gemini_sandbox_api_url',
    'gemini_sandbox_client_id',
    'gemini_sandbox_client_secret',
    'gemini_sandbox_fee_address',
    'gemini_sandbox_oauth_url',
    'google_default_client_id',
    'google_default_client_secret',
    'msan_track_origins',
    'rewards_grant_dev_endpoint',
    'rewards_grant_prod_endpoint',
    'rewards_grant_staging_endpoint',
    'safebrowsing_api_endpoint',
    'sardine_client_id',
    'sardine_client_secret',
    'service_key_aichat',
    'service_key_stt',
    'sparkle_dsa_private_key_file',
    'sparkle_eddsa_private_key',
    'sparkle_eddsa_public_key',
    'updater_dev_endpoint',
    'updater_prod_endpoint',
    'uphold_production_api_url',
    'uphold_production_client_id',
    'uphold_production_client_secret',
    'uphold_production_fee_address',
    'uphold_production_oauth_url',
    'uphold_sandbox_api_url',
    'uphold_sandbox_client_id',
    'uphold_sandbox_client_secret',
    'uphold_sandbox_fee_address',
    'uphold_sandbox_oauth_url',
    'use_prebuilt_omaha4',
    'webcompat_report_api_endpoint',
    'zebpay_production_api_url',
    'zebpay_production_client_id',
    'zebpay_production_client_secret',
    'zebpay_production_oauth_url',
    'zebpay_sandbox_api_url',
    'zebpay_sandbox_client_id',
    'zebpay_sandbox_client_secret',
    'zebpay_sandbox_oauth_url',
    'use_clang_coverage',
    'coverage_instrumentation_input_file',
  ]
}

Config.prototype.isReleaseBuild = function () {
  return this.buildConfig === 'Release'
}

Config.prototype.isBraveReleaseBuild = function () {
  const isBraveReleaseBuildValue = getEnvConfig(['is_brave_release_build'])
  if (isBraveReleaseBuildValue !== undefined) {
    assert(
      isBraveReleaseBuildValue === 0 || isBraveReleaseBuildValue === 1,
      'Bad is_brave_release_build value (should be 0 or 1)',
    )
    return isBraveReleaseBuildValue === 1
  }

  return false
}

Config.prototype.isComponentBuild = function () {
  return this.buildConfig === 'Debug' || this.buildConfig === 'Component'
}

Config.prototype.isDebug = function () {
  return this.buildConfig === 'Debug'
}

Config.prototype.enableCDMHostVerification = function () {
  const enable =
    this.buildConfig === 'Release'
    && process.platform !== 'linux'
    && this.sign_widevine_cert !== ''
    && this.sign_widevine_key !== ''
    && this.sign_widevine_passwd !== ''
    && fs.existsSync(this.signature_generator)
  if (enable) {
    console.log('Widevine cdm host verification is enabled')
  } else {
    console.log('Widevine cdm host verification is disabled')
  }
  return enable
}

Config.prototype.isAsan = function () {
  if (this.is_asan) {
    return true
  }
  return false
}

Config.prototype.isOfficialBuild = function () {
  return (
    this.isReleaseBuild() && !this.isAsan() && !this.is_msan && !this.is_ubsan
  )
}

Config.prototype.getBraveLogoIconName = function () {
  let iconName = 'brave-icon-dev-color.svg'
  if (this.isBraveReleaseBuild()) {
    if (this.channel === 'beta') {
      iconName = 'brave-icon-beta-color.svg'
    } else if (this.channel === 'nightly') {
      iconName = 'brave-icon-nightly-color.svg'
    } else {
      iconName = 'brave-icon-release-color.svg'
    }
  }
  return iconName
}

Config.prototype.buildArgs = function () {
  const version = this.braveVersion
  let versionParts = version.split('+')[0]
  versionParts = versionParts.split('.')

  let args = {
    'import("//brave/build/args/brave_defaults.gni")': null,
    is_asan: this.isAsan(),
    enable_full_stack_frames_for_profiling: this.isAsan(),
    v8_enable_verify_heap: this.isAsan(),
    is_ubsan: this.is_ubsan,
    is_ubsan_vptr: this.is_ubsan,
    is_ubsan_no_recover: this.is_ubsan,
    is_msan: this.is_msan,
    // TODO: Re-enable when chromium_src overrides work for files in relative
    // paths like widevine_cmdm_compoennt_installer.cc
    // use_jumbo_build: !this.officialBuild,
    is_component_build: this.isComponentBuild(),
    is_universal_binary: this.isUniversalBinary,
    target_cpu: this.targetArch,
    is_official_build: this.isOfficialBuild(),
    is_debug: this.isDebug(),
    brave_channel: this.channel,
    brave_version_major: versionParts[0],
    brave_version_minor: versionParts[1],
    brave_version_build: versionParts[2],
    chrome_version_string: this.chromeVersion,
    enable_hangout_services_extension: this.enable_hangout_services_extension,
    enable_cdm_host_verification: this.enableCDMHostVerification(),
    skip_signing: !this.shouldSign(),
    use_remoteexec: this.useRemoteExec,
    use_reclient: this.useRemoteExec,
    use_siso: this.useSiso,
    use_libfuzzer: this.use_libfuzzer,
    enable_update_notifications: this.isOfficialBuild(),
    v8_enable_drumbrake: false,
  }

  if (this.targetOS !== 'ios') {
    args['import("//brave/build/args/blink_platform_defaults.gni")'] = null
  } else {
    args['import("//brave/build/args/ios_defaults.gni")'] = null
  }
  if (this.targetOS === 'android') {
    args['import("//brave/build/args/android_defaults.gni")'] = null
  }
  if (this.targetOS !== 'ios' && this.targetOS !== 'android') {
    args['import("//brave/build/args/desktop_defaults.gni")'] = null
  }

  for (const key of this.forwardEnvArgsToGn) {
    args[key] = getEnvConfig([key])
  }

  if (this.isOfficialBuild()) {
    args.enable_updater = true
  }

  if (!this.isBraveReleaseBuild()) {
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

  if (this.ignorePatchVersionNumber) {
    assert(!this.isBraveReleaseBuild())

    // Allow dummy LASTCHANGE to be set. When the real LASTCHANGE is used, ~2300
    // targets are rebuilt with each version bump.
    args.use_dummy_lastchange = getEnvConfig(['use_dummy_lastchange'], true)
  }

  if (this.shouldSign()) {
    if (this.targetOS === 'mac') {
      args.mac_signing_identifier = this.mac_signing_identifier
      args.mac_installer_signing_identifier =
        this.mac_installer_signing_identifier
      args.mac_signing_keychain = this.mac_signing_keychain
      if (this.notarize) {
        args.notarize = true
        args.notary_user = this.notary_user
        args.notary_password = this.notary_password
      }
    } else if (this.targetOS === 'android') {
      args.brave_android_keystore_path = this.braveAndroidKeystorePath
      args.brave_android_keystore_name = this.braveAndroidKeystoreName
      args.brave_android_keystore_password = this.braveAndroidKeystorePassword
      args.brave_android_key_password = this.braveAndroidKeyPassword
      if (this.braveAndroidPkcs11Provider && this.braveAndroidPkcs11Alias) {
        args.brave_android_pkcs11_provider = this.braveAndroidPkcs11Provider
        args.brave_android_pkcs11_alias = this.braveAndroidPkcs11Alias
      }
    }
  }

  if (this.build_omaha) {
    args.build_omaha = this.build_omaha
    args.tag_ap = this.tag_ap
    if (this.tag_installdataindex) {
      args.tag_installdataindex = this.tag_installdataindex
    }
  }

  if (
    (process.platform === 'win32' || process.platform === 'darwin')
    && this.build_delta_installer
  ) {
    assert(
      this.last_chrome_installer,
      'Need last_chrome_installer args for building delta installer',
    )
    args.build_delta_installer = true
    args.last_chrome_installer = this.last_chrome_installer
  }

  if (process.platform === 'darwin') {
    args.allow_runtime_configurable_key_storage = true
  }

  if (
    this.isDebug()
    && !this.isComponentBuild()
    && this.targetOS !== 'ios'
    && this.targetOS !== 'android'
  ) {
    args.enable_profiling = true
  }

  if (!this.useSiso) {
    if (this.useRemoteExec) {
      args.reclient_bin_dir = path.join(this.nativeRedirectCCDir)
    } else {
      args.cc_wrapper = path.join(this.nativeRedirectCCDir, 'redirect_cc')
    }
  }

  // Adjust symbol_level in Linux builds:
  // 1. Set minimal symbol level to workaround size restrictions: on Linux x86,
  //    ELF32 cannot be > 4GiB.
  // 2. Enable symbols in Static builds. By default symbol_level is 0 in this
  //    configuration. symbol_level = 2 cannot be used because of "relocation
  //    R_X86_64_32 out of range" errors.
  if (
    this.targetOS === 'linux'
    && (this.targetArch === 'x86'
      || (!this.isDebug()
        && !this.isComponentBuild()
        && !this.isReleaseBuild()))
  ) {
    args.symbol_level = 1
  }

  if (this.use_clang_coverage) {
    const buildDir = path.relative(this.srcDir, this.outputDir)
    args.use_clang_coverage = true
    args.coverage_instrumentation_input_file = `//${buildDir}/files-to-instrument.txt`
  }

  // For Linux Release builds, upstream doesn't want to use symbol_level = 2
  // unless use_debug_fission is set. However, they don't set it when a
  // cc_wrapper is used. Since we use cc_wrapper we need to set it manually.
  if (this.targetOS === 'linux' && this.isReleaseBuild()) {
    args.use_debug_fission = true
  }

  if (
    this.targetOS === 'mac'
    && fs.existsSync(
      path.join(
        this.srcDir,
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

  if (this.targetOS === 'linux') {
    if (this.targetArch !== 'x86') {
      // Include vaapi support
      // TODO: Consider setting use_vaapi_x11 instead of use_vaapi. Also
      // consider enabling it for x86 builds. See
      // https://github.com/brave/brave-browser/issues/1024#issuecomment-1175397914
      args.use_vaapi = true
    }
  }

  if (['android', 'linux', 'mac'].includes(this.targetOS)) {
    // LSAN only works with ASAN and has very low overhead.
    args.is_lsan = args.is_asan
  }

  // Devtools: Now we patch devtools frontend, so it is useful to see
  // if something goes wrong on CI builds.
  if (this.targetOS !== 'android' && this.targetOS !== 'ios' && this.isCI) {
    args.devtools_skip_typecheck = false
  }

  if (this.targetOS) {
    args.target_os = this.targetOS
  }

  if (this.targetOS === 'android') {
    args.android_channel = this.channel
    if (!this.isReleaseBuild()) {
      args.android_channel = 'default'
      args.chrome_public_manifest_package = 'com.brave.browser_default'
    } else if (this.channel === '') {
      args.android_channel = 'stable'
      args.chrome_public_manifest_package = 'com.brave.browser'
    } else if (this.channel === 'beta') {
      args.chrome_public_manifest_package = 'com.brave.browser_beta'
    } else if (this.channel === 'dev') {
      args.chrome_public_manifest_package = 'com.brave.browser_dev'
    } else if (this.channel === 'nightly') {
      args.android_channel = 'canary'
      args.chrome_public_manifest_package = 'com.brave.browser_nightly'
    }
    // exclude_unwind_tables is inherited form upstream and is false for any
    // Android build

    args.target_android_base = this.targetAndroidBase
    args.target_android_output_format =
      this.targetAndroidOutputFormat
      || (this.buildConfig === 'Release' ? 'aab' : 'apk')
    args.android_override_version_name = this.androidOverrideVersionName

    args.brave_android_developer_options_code =
      this.braveAndroidDeveloperOptionsCode
    args.brave_safebrowsing_api_key = this.braveAndroidSafeBrowsingApiKey

    args.android_aab_to_apk = this.androidAabToApk

    if (
      args.target_android_output_format === 'apk'
      && (this.targetArch === 'arm64' || this.targetArch === 'x64')
    ) {
      // We want to have both 32 and 64 bit native libs in arm64/x64 apks
      // Starting from cr136 it is defaulted to false.
      // For local build you can add --gn=enable_android_secondary_abi:false
      // to have only 64 bit libs.
      args.enable_android_secondary_abi = true
    }

    if (this.isCI && !this.isOfficialBuild()) {
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

  if (this.targetOS === 'ios') {
    if (this.targetEnvironment) {
      args.target_environment = this.targetEnvironment
    }
    if (this.braveIOSMarketingPatchVersion) {
      args.brave_ios_marketing_version_patch =
        this.braveIOSMarketingPatchVersion
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

    args.brave_ios_developer_options_code = this.braveIOSDeveloperOptionsCode

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
    delete args.gemini_production_client_id
    delete args.gemini_production_client_secret
    delete args.gemini_production_fee_address
    delete args.gemini_production_oauth_url
    delete args.gemini_sandbox_api_url
    delete args.gemini_sandbox_client_id
    delete args.gemini_sandbox_client_secret
    delete args.gemini_sandbox_fee_address
    delete args.gemini_sandbox_oauth_url
    delete args.uphold_production_api_url
    delete args.uphold_production_client_id
    delete args.uphold_production_client_secret
    delete args.uphold_production_fee_address
    delete args.uphold_production_oauth_url
    delete args.uphold_sandbox_api_url
    delete args.uphold_sandbox_client_id
    delete args.uphold_sandbox_client_secret
    delete args.uphold_sandbox_fee_address
    delete args.uphold_sandbox_oauth_url
    delete args.zebpay_production_api_url
    delete args.zebpay_production_client_id
    delete args.zebpay_production_client_secret
    delete args.zebpay_production_oauth_url
    delete args.zebpay_sandbox_api_url
    delete args.zebpay_sandbox_client_id
    delete args.zebpay_sandbox_client_secret
    delete args.zebpay_sandbox_oauth_url
    delete args.use_blink_v8_binding_new_idl_interface
    delete args.v8_enable_verify_heap
    delete args.service_key_stt
  }

  args = Object.assign(args, this.extraGnArgs)
  return args
}

Config.prototype.shouldSign = function () {
  if (this.skip_signing || this.isComponentBuild() || this.targetOS === 'ios') {
    return false
  }

  if (this.targetOS === 'android') {
    return this.braveAndroidKeystorePath !== undefined
  }

  if (this.targetOS === 'mac') {
    return this.mac_signing_identifier !== undefined
  }

  if (process.platform === 'win32') {
    return (
      process.env.CERT !== undefined
      || process.env.AUTHENTICODE_HASH !== undefined
      || process.env.SIGNTOOL_ARGS !== undefined
    )
  }

  return false
}

Config.prototype.addToPath = function (oldPath, addPath, prepend = false) {
  const newPath = oldPath ? oldPath.split(path.delimiter) : []
  if (newPath.includes(addPath)) {
    return oldPath
  }
  if (prepend) {
    newPath.unshift(addPath)
  } else {
    newPath.push(addPath)
  }
  return newPath.join(path.delimiter)
}

Config.prototype.addPathToEnv = function (env, addPath, prepend = false) {
  // cmd.exe uses Path instead of PATH so just set both
  env.Path && (env.Path = this.addToPath(env.Path, addPath, prepend))
  env.PATH && (env.PATH = this.addToPath(env.PATH, addPath, prepend))
  return env
}

Config.prototype.addPythonPathToEnv = function (env, addPath) {
  env.PYTHONPATH = this.addToPath(env.PYTHONPATH, addPath)
  return env
}

Config.prototype.getProjectVersion = function (projectName) {
  return (
    getEnvConfig(['projects', projectName, 'revision'])
    || getEnvConfig(['projects', projectName, 'tag'])
    || getEnvConfig(['projects', projectName, 'branch'])
  )
}

Config.prototype.getProjectRef = function (
  projectName,
  defaultValue = 'origin/master',
) {
  const revision = getEnvConfig(['projects', projectName, 'revision'])
  if (revision) {
    return revision
  }

  const tag = getEnvConfig(['projects', projectName, 'tag'])
  if (tag) {
    return `refs/tags/${tag}`
  }

  let branch = getEnvConfig(['projects', projectName, 'branch'])
  if (branch) {
    return `origin/${branch}`
  }

  return defaultValue
}

Config.prototype.updateInternal = function (options) {
  if (options.universal) {
    this.targetArch = 'arm64'
    this.isUniversalBinary = true
  }

  if (options.target_cpu) {
    options.target_arch = options.target_cpu
  }

  if (options.target_arch === 'x86') {
    this.targetArch = options.target_arch
    this.gypTargetArch = 'ia32'
  } else if (options.target_arch === 'ia32') {
    this.targetArch = 'x86'
    this.gypTargetArch = options.target_arch
  } else if (options.target_arch) {
    this.targetArch = options.target_arch
  }

  if (options.target_os) {
    // Handle non-standard target_os values as they are used on CI currently and
    // it's easier to support them as is instead of rewriting the CI scripts.
    if (options.target_os === 'macos') {
      this.targetOS = 'mac'
    } else if (options.target_os === 'windows') {
      this.targetOS = 'win'
    } else {
      this.targetOS = options.target_os
    }
  }

  if (this.targetOS === 'android') {
    if (options.target_android_base) {
      this.targetAndroidBase = options.target_android_base
    }
    if (options.target_android_output_format) {
      this.targetAndroidOutputFormat = options.target_android_output_format
    }
    if (options.android_override_version_name) {
      this.androidOverrideVersionName = options.android_override_version_name
    }
    if (options.android_aab_to_apk) {
      this.androidAabToApk = options.android_aab_to_apk
    }
  }

  if (this.targetOS === 'ios' && options.target_environment) {
    this.targetEnvironment = options.target_environment
  }

  if (options.build_config) {
    this.buildConfig = options.build_config
  }

  if (options.is_asan) {
    this.is_asan = true
  } else {
    this.is_asan = false
  }

  if (options.use_clang_coverage) {
    this.use_clang_coverage = true
  }

  if (options.is_ubsan) {
    this.is_ubsan = true
  }

  if (options.use_remoteexec !== undefined) {
    this.useRemoteExec = options.use_remoteexec
  }

  if (options.offline) {
    this.offline = true
  }

  if (options.force_gn_gen) {
    this.force_gn_gen = true
  } else {
    this.force_gn_gen = false
  }

  if (options.C) {
    this.__outputDir = options.C
  }

  if (options.gclient_file && options.gclient_file !== 'default') {
    this.gClientFile = options.gclient_file
  }

  if (options.channel) {
    this.channel = options.channel
  } else if (this.buildConfig === 'Release') {
    this.channel = 'release'
  }

  if (this.channel === 'release') {
    // empty for release channel
    this.channel = ''
  }

  if (options.build_omaha) {
    assert(process.platform === 'win32')
    this.build_omaha = true
    assert(options.tag_ap, '--tag_ap is required for --build_omaha')
  }

  if (options.tag_ap) {
    assert(options.build_omaha, '--tag_ap requires --build_omaha')
    this.tag_ap = options.tag_ap
  }

  if (options.tag_installdataindex) {
    assert(options.build_omaha, '--tag_installdataindex requires --build_omaha')
    this.tag_installdataindex = options.tag_installdataindex
  }

  if (options.skip_signing) {
    this.skip_signing = true
  }

  if (options.build_delta_installer) {
    this.build_delta_installer = true
    this.last_chrome_installer = options.last_chrome_installer
  }

  if (options.mac_signing_identifier) {
    this.mac_signing_identifier = options.mac_signing_identifier
  }

  if (options.mac_installer_signing_identifier) {
    this.mac_installer_signing_identifier =
      options.mac_installer_signing_identifier
  }

  if (options.mac_signing_keychain) {
    this.mac_signing_keychain = options.mac_signing_keychain
  }

  if (options.notarize) {
    this.notarize = true
  }

  if (options.gclient_verbose) {
    this.gClientVerbose = options.gclient_verbose
  }

  if (options.ignore_compile_failure) {
    this.ignore_compile_failure = true
  }

  if (options.xcode_gen) {
    assert(process.platform === 'darwin' || options.target_os === 'ios')
    if (options.xcode_gen === 'ios') {
      this.xcode_gen_target = '//brave/ios:*'
    } else {
      this.xcode_gen_target = options.xcode_gen
    }
  }

  if (options.gn) {
    parseExtraInputs(options.gn, this.extraGnArgs, (args, key, value) => {
      try {
        value = JSON.parse(value)
      } catch (e) {
        // On parse error, leave value as string.
      }
      args[key] = value
    })
  }

  if (options.ninja) {
    parseExtraInputs(options.ninja, this.extraNinjaOpts, (opts, key, value) => {
      // Workaround siso unable to handle -j if REAPI is not configured.
      if (key === 'j' && this.useSiso) {
        this.sisoJobsLimit = parseInt(value)
        return
      }
      opts.push(`-${key}`)
      if (value) {
        opts.push(value)
      }
    })
  }

  if (this.offline || !this.useRemoteExec) {
    // Pass '--offline' also when '--use_remoteexec' is not set to disable RBE
    // detect in autoninja when doing local builds.
    this.extraNinjaOpts.push('--offline')
  }

  if (options.target) {
    this.buildTargets = options.target.split(',')
  }

  if (options.use_libfuzzer) {
    this.use_libfuzzer = options.use_libfuzzer
  }

  if (options.pkcs11Provider) {
    this.braveAndroidPkcs11Provider = options.pkcs11Provider
  }

  if (options.pkcs11Alias) {
    this.braveAndroidPkcs11Alias = options.pkcs11Alias
  }
}

Config.prototype.fromGnArgs = function (options) {
  const gnArgs = readArgsGn(this.srcDir, options.C)
  Log.warn(
    '--no-gn-gen is experimental and only gn args that match command '
      + 'line options will be processed',
  )
  this.updateInternal(Object.assign({}, gnArgs, options))
  assert(!this.isCI)
}

Config.prototype.update = function (options) {
  if (this.use_no_gn_gen) {
    this.fromGnArgs(options)
  } else {
    this.updateInternal(options)
  }
}

Object.defineProperty(Config.prototype, 'targetOS', {
  get: function () {
    if (this._targetOS) {
      return this._targetOS
    }
    return this.hostOS
  },
  set: function (value) {
    this._targetOS = value
    if (this._targetOS) {
      const supportedOS = ['android', 'ios', 'linux', 'mac', 'win']
      assert(
        supportedOS.includes(this._targetOS),
        `Unsupported target_os value: ${
          this._targetOS
        }, supported values: ${supportedOS.join(', ')}`,
      )
    }
  },
})

Config.prototype.getCachePath = function () {
  return this.git_cache_path || process.env.GIT_CACHE_PATH
}

Config.prototype.isIOS = function () {
  return this.targetOS === 'ios'
}

Config.prototype.isAndroid = function () {
  return this.targetOS === 'android'
}

Config.prototype.isMobile = function () {
  return this.isIOS() || this.isAndroid()
}

Object.defineProperty(Config.prototype, 'defaultOptions', {
  get: function () {
    let env = Object.assign({}, process.env)
    env = this.addPathToEnv(
      env,
      path.join(this.depotToolsDir, 'python-bin'),
      true,
    )
    env = this.addPathToEnv(
      env,
      path.join(this.depotToolsDir, 'python2-bin'),
      true,
    )
    env = this.addPathToEnv(
      env,
      path.join(this.srcDir, 'third_party', 'rust-toolchain', 'bin'),
      true,
    )
    env = this.addPathToEnv(env, this.depotToolsDir, true)
    if (this.targetOS === 'mac' && process.platform !== 'darwin') {
      const crossCompilePath = path.join(
        this.srcDir,
        'brave',
        'build',
        'mac',
        'cross_compile',
        'path',
      )
      env = this.addPathToEnv(env, crossCompilePath, true)
    }
    const pythonPaths = [
      ['brave', 'script'],
      ['tools', 'grit', 'grit', 'extern'],
      ['brave', 'vendor', 'requests'],
      ['brave', 'third_party', 'cryptography'],
      ['brave', 'third_party', 'macholib'],
      ['build'],
      ['third_party', 'depot_tools'],
    ]
    pythonPaths.forEach((p) => {
      env = this.addPythonPathToEnv(env, path.join(this.srcDir, ...p))
    })
    env.PYTHONUNBUFFERED = '1'
    if (process.platform === 'win32') {
      // UTF-8 is default on Linux/Mac, but on Windows CP1252 is used in most
      // cases. This var makes Python use UTF-8 if encoding is not set
      // explicitly in calls such as `open()`.
      // https://peps.python.org/pep-0540/
      env.PYTHONUTF8 = '1'
    }
    env.TARGET_ARCH = this.gypTargetArch // for brave scripts
    env.RUSTUP_HOME = path.join(this.srcDir, 'third_party', 'rust-toolchain')
    // Fix `gclient runhooks` - broken since depot_tools a7b20b34f85432b5958963b75edcedfef9cf01fd
    env.GSUTIL_ENABLE_LUCI_AUTH = '0'

    if (this.channel) {
      env.BRAVE_CHANNEL = this.channel
    }

    if (!this.useBraveHermeticToolchain) {
      env.DEPOT_TOOLS_WIN_TOOLCHAIN = '0'
    } else {
      // Use hermetic toolchain only internally.
      env.USE_BRAVE_HERMETIC_TOOLCHAIN = '1'
      env.DEPOT_TOOLS_WIN_TOOLCHAIN = '1'
      env.GYP_MSVS_HASH_e4305f407e = '7a2a21dbe7'
      env.DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL = `${this.internalDepsUrl}/windows-hermetic-toolchain/`
    }

    if (this.getCachePath()) {
      env.GIT_CACHE_PATH = path.join(this.getCachePath())
    }

    if (this.rbeService) {
      // These env vars are required during `sync` stage.
      env.RBE_service = env.RBE_service || this.rbeService
      if (this.rbeTlsClientAuthCert && this.rbeTlsClientAuthKey) {
        env.RBE_tls_client_auth_cert =
          env.RBE_tls_client_auth_cert || this.rbeTlsClientAuthCert
        env.RBE_tls_client_auth_key =
          env.RBE_tls_client_auth_key || this.rbeTlsClientAuthKey
        env.RBE_service_no_auth = env.RBE_service_no_auth || true
        env.RBE_use_application_default_credentials =
          env.RBE_use_application_default_credentials || true
      }
    }

    // These env vars are required during `build` stage.
    if (this.useRemoteExec) {
      // Restrict remote execution to 160 parallel jobs.
      const kRemoteLimit = 160

      // Prevent depot_tools from setting lower timeouts.
      const kRbeTimeout = '10m'
      env.RBE_exec_timeout = env.RBE_exec_timeout || kRbeTimeout
      env.RBE_reclient_timeout = env.RBE_reclient_timeout || kRbeTimeout

      // Autoninja generates -j value when RBE is enabled, adjust limits for
      // Brave-specific setup.
      env.NINJA_CORE_MULTIPLIER = Math.min(20, env.NINJA_CORE_MULTIPLIER || 20)
      env.NINJA_CORE_LIMIT = Math.min(
        kRemoteLimit,
        env.NINJA_CORE_LIMIT || kRemoteLimit,
      )

      // Siso has its own limits for remote execution that do not depend on
      // NINJA_CORE_* values. Set those limits separately. See docs for more
      // details:
      // https://chromium.googlesource.com/build/+/refs/heads/main/siso/docs/environment_variables.md#siso_limits
      const defaultSisoLimits = {
        local: this.sisoJobsLimit,
        remote: this.sisoJobsLimit || kRemoteLimit,
        rewrap: this.sisoJobsLimit || kRemoteLimit,
      }
      // Parse SISO_LIMITS from env if set.
      const envSisoLimits = new Map(
        env.SISO_LIMITS?.split(',').map((item) => item.split('=')) || [],
      )
      // Merge defaultSisoLimits with envSisoLimits ensuring that the values are
      // not greater than the default values.
      Object.entries(defaultSisoLimits).forEach(([key, defaultValue]) => {
        if (defaultValue === undefined) {
          return
        }
        const valueFromEnv = parseInt(envSisoLimits.get(key)) || defaultValue
        envSisoLimits.set(key, Math.min(defaultValue, valueFromEnv))
      })
      // Set SISO_LIMITS env var.
      env.SISO_LIMITS = Array.from(envSisoLimits.entries())
        .map(([key, value]) => `${key}=${value}`)
        .join(',')

      if (this.offline) {
        // Use all local resources in offline mode. RBE_local_resource_fraction
        // can be set to a lower value for racing mode, but in offline mode we
        // want to use all cores.
        env.RBE_local_resource_fraction = '1.0'
      }
    }

    if (this.isCI) {
      // Enables autoninja to show build speed and final stats on finish.
      env.NINJA_SUMMARIZE_BUILD = 1
    }

    if (process.platform === 'linux') {
      env.LLVM_DOWNLOAD_GOLD_PLUGIN = '1'
    }

    if (process.platform === 'win32') {
      // Disable vcvarsall.bat telemetry.
      env.VSCMD_SKIP_SENDTELEMETRY = '1'
    }

    if (this.isCI && this.skip_download_rust_toolchain_aux) {
      env.SKIP_DOWNLOAD_RUST_TOOLCHAIN_AUX = '1'
    }

    // TeamCity displays only stderr on the "Build Problems" page when an error
    // occurs. By redirecting stdout to stderr, we ensure that all outputs from
    // external processes are visible in case of a failure.
    const stdio = this.isTeamcity
      ? ['inherit', process.stderr, 'inherit']
      : 'inherit'

    return {
      env,
      stdio: stdio,
      cwd: this.srcDir,
      git_cwd: '.',
    }
  },
})

Object.defineProperty(Config.prototype, 'outputDir', {
  get: function () {
    if (this.use_no_gn_gen && this.__outputDir == null) {
      Log.error(`You must specify output directory with -C with use_no_gn_gen`)
      process.exit(1)
    }

    const baseDir = path.join(this.srcDir, 'out')
    if (this.__outputDir) {
      if (path.isAbsolute(this.__outputDir)) {
        return this.__outputDir
      }
      return path.join(baseDir, this.__outputDir)
    }

    let buildConfigDir = this.buildConfig
    if (this.targetArch && this.targetArch !== 'x64') {
      buildConfigDir = buildConfigDir + '_' + this.targetArch
    }
    if (this.targetOS && this.targetOS !== this.hostOS) {
      buildConfigDir = this.targetOS + '_' + buildConfigDir
    }
    if (
      this.targetOS === 'ios'
      && this.targetEnvironment
      && this.targetEnvironment !== 'device'
    ) {
      buildConfigDir = buildConfigDir + '_' + this.targetEnvironment
    }
    if (this.isChromium) {
      buildConfigDir = buildConfigDir + '_chromium'
    }

    return path.join(baseDir, buildConfigDir)
  },
  set: function (outputDir) {
    return (this.__outputDir = outputDir)
  },
})

Object.defineProperty(Config.prototype, 'useSiso', {
  get: function () {
    return getEnvConfig(
      ['use_siso'],
      // * iOS fails in siso+reproxy mode because of incorrect handling of
      //   input_root_absolute_path value.
      !this.isIOS(),
    )
  },
})

module.exports = new Config()
