// Copyright (c) 2016 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const path = require('path')
const fs = require('fs')
const os = require('os')
const assert = require('assert')
const { spawnSync } = require('child_process')

let npmCommand = 'npm'
if (process.platform === 'win32') {
  npmCommand += '.cmd'
}
let NpmConfig = null

let dirName = __dirname
// Use fs.realpathSync to normalize the path(__dirname could be c:\.. or C:\..).
if (process.platform === 'win32') {
  dirName = fs.realpathSync.native(dirName)
}
const rootDir = path.resolve(dirName, '..', '..', '..', '..', '..')
const braveCoreDir = path.join(rootDir, 'src', 'brave')

const run = (cmd, args = []) => {
  const prog = spawnSync(cmd, args)
  if (prog.status !== 0) {
    console.log(prog.stdout && prog.stdout.toString())
    console.error(prog.stderr && prog.stderr.toString())
    process.exit(1)
  }
  return prog
}

var packageConfig = function (key, sourceDir = braveCoreDir) {
  let packages = { config: {} }
  const configAbsolutePath = path.join(sourceDir, 'package.json')
  if (fs.existsSync(configAbsolutePath)) {
    packages = require(path.relative(__dirname, configAbsolutePath))
  }

  // packages.config should include version string.
  let obj = Object.assign({}, packages.config, { version: packages.version })
  for (var i = 0, len = key.length; i < len; i++) {
    if (!obj) {
      return obj
    }
    obj = obj[key[i]]
  }
  return obj
}

const getNPMConfig = (key, default_value = undefined) => {
  if (!NpmConfig) {
    const list = run(npmCommand, ['config', 'list', '--json', '--userconfig=' + path.join(rootDir, '.npmrc')])
    NpmConfig = JSON.parse(list.stdout.toString())
  }

  // NpmConfig has the multiple copy of the same variable: one from .npmrc
  // (that we want to) and one from the environment.
  // https://docs.npmjs.com/cli/v7/using-npm/config#environment-variables
  const npmConfigValue = NpmConfig[key.join('_')]
  if (npmConfigValue !== undefined)
    return npmConfigValue

  // Shouldn't be used in general but added for backward compatibilty.
  const npmConfigDeprecatedValue = NpmConfig[key.join('-').replace(/_/g, '-')]
  if (npmConfigDeprecatedValue !== undefined)
    return npmConfigDeprecatedValue

  const packageConfigValue = packageConfig(key)
  if (packageConfigValue !== undefined)
    return packageConfigValue

  return default_value
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

const Config = function () {
  this.sardineClientId = getNPMConfig(['sardine_client_id']) || ''
  this.sardineClientSecret = getNPMConfig(['sardine_client_secret']) || ''
  this.defaultBuildConfig = 'Component'
  this.buildConfig = this.defaultBuildConfig
  this.signTarget = 'sign_app'
  this.buildTarget = 'brave'
  this.rootDir = rootDir
  this.isUniversalBinary = false
  this.scriptDir = path.join(this.rootDir, 'scripts')
  this.srcDir = path.join(this.rootDir, 'src')
  this.chromeVersion = this.getProjectVersion('chrome')
  this.chromiumRepo = getNPMConfig(['projects', 'chrome', 'repository', 'url'])
  this.braveCoreDir = braveCoreDir
  this.buildToolsDir = path.join(this.srcDir, 'build')
  this.resourcesDir = path.join(this.rootDir, 'resources')
  this.depotToolsDir = path.join(this.braveCoreDir, 'vendor', 'depot_tools')
  this.defaultGClientFile = path.join(this.rootDir, '.gclient')
  this.gClientFile = process.env.BRAVE_GCLIENT_FILE || this.defaultGClientFile
  this.gClientVerbose = getNPMConfig(['gclient_verbose']) || false
  this.targetArch = getNPMConfig(['target_arch']) || process.arch
  this.targetOS = getNPMConfig(['target_os'])
  this.targetEnvironment = getNPMConfig(['target_environment'])
  this.gypTargetArch = 'x64'
  this.targetAndroidBase = 'classic'
  this.braveGoogleApiKey = getNPMConfig(['brave_google_api_key']) || 'AIzaSyAREPLACEWITHYOUROWNGOOGLEAPIKEY2Q'
  this.googleApiEndpoint = getNPMConfig(['brave_google_api_endpoint']) || 'https://www.googleapis.com/geolocation/v1/geolocate?key='
  this.googleDefaultClientId = getNPMConfig(['google_default_client_id']) || ''
  this.googleDefaultClientSecret = getNPMConfig(['google_default_client_secret']) || ''
  this.braveServicesKey = getNPMConfig(['brave_services_key']) || ''
  this.infuraProjectId = getNPMConfig(['brave_infura_project_id']) || ''
  this.bitflyerClientId = getNPMConfig(['bitflyer_client_id']) || ''
  this.bitflyerClientSecret = getNPMConfig(['bitflyer_client_secret']) || ''
  this.bitflyerStagingClientId = getNPMConfig(['bitflyer_staging_client_id']) || ''
  this.bitflyerStagingClientSecret = getNPMConfig(['bitflyer_staging_client_secret']) || ''
  this.bitflyerStagingUrl = getNPMConfig(['bitflyer_staging_url']) || ''
  this.geminiApiUrl = getNPMConfig(['gemini_api_url']) || ''
  this.geminiApiStagingUrl = getNPMConfig(['gemini_api_staging_url']) || ''
  this.geminiOauthUrl = getNPMConfig(['gemini_oauth_url']) || ''
  this.geminiOauthStagingUrl = getNPMConfig(['gemini_oauth_staging_url']) || ''
  this.geminiWalletClientId = getNPMConfig(['gemini_wallet_client_id']) || ''
  this.geminiWalletClientSecret = getNPMConfig(['gemini_wallet_client_secret']) || ''
  this.geminiWalletStagingClientId = getNPMConfig(['gemini_wallet_staging_client_id']) || ''
  this.geminiWalletStagingClientSecret = getNPMConfig(['gemini_wallet_staging_client_secret']) || ''
  this.upholdClientId = getNPMConfig(['uphold_client_id']) || ''
  this.upholdClientSecret = getNPMConfig(['uphold_client_secret']) || ''
  this.upholdStagingClientId = getNPMConfig(['uphold_staging_client_id']) || ''
  this.upholdStagingClientSecret = getNPMConfig(['uphold_staging_client_secret']) || ''
  this.braveSyncEndpoint = getNPMConfig(['brave_sync_endpoint']) || ''
  this.safeBrowsingApiEndpoint = getNPMConfig(['safebrowsing_api_endpoint']) || ''
  this.updaterProdEndpoint = getNPMConfig(['updater_prod_endpoint']) || ''
  this.updaterDevEndpoint = getNPMConfig(['updater_dev_endpoint']) || ''
  this.webcompatReportApiEndpoint = getNPMConfig(['webcompat_report_api_endpoint']) || 'https://webcompat.brave.com/1/webcompat'
  this.rewardsGrantDevEndpoint = getNPMConfig(['rewards_grant_dev_endpoint']) || ''
  this.rewardsGrantStagingEndpoint = getNPMConfig(['rewards_grant_staging_endpoint']) || ''
  this.rewardsGrantProdEndpoint = getNPMConfig(['rewards_grant_prod_endpoint']) || ''
  this.braveVersion = packageConfig(['version']) || '0.0.0'
  this.androidOverrideVersionName = this.braveVersion
  this.releaseTag = this.braveVersion.split('+')[0]
  this.mac_signing_identifier = getNPMConfig(['mac_signing_identifier'])
  this.mac_installer_signing_identifier = getNPMConfig(['mac_installer_signing_identifier']) || ''
  this.mac_signing_keychain = getNPMConfig(['mac_signing_keychain']) || 'login'
  this.sparkleDSAPrivateKeyFile = getNPMConfig(['sparkle_dsa_private_key_file']) || ''
  this.sparkleEdDSAPrivateKey = getNPMConfig(['sparkle_eddsa_private_key']) || ''
  this.sparkleEdDSAPublicKey = getNPMConfig(['sparkle_eddsa_public_key']) || ''
  this.notary_user = getNPMConfig(['notary_user']) || ''
  this.notary_password = getNPMConfig(['notary_password']) || ''
  this.channel = 'development'
  this.git_cache_path = getNPMConfig(['git_cache_path'])
  this.sccache = getNPMConfig(['sccache'])
  this.gomaServerHost = getNPMConfig(['goma_server_host'])
  this.isCI = process.env.BUILD_ID !== undefined
  this.braveStatsApiKey = getNPMConfig(['brave_stats_api_key']) || ''
  this.braveStatsUpdaterUrl = getNPMConfig(['brave_stats_updater_url']) || ''
  this.ignore_compile_failure = false
  this.enable_hangout_services_extension = true
  this.enable_pseudolocales = false
  this.sign_widevine_cert = process.env.SIGN_WIDEVINE_CERT || ''
  this.sign_widevine_key = process.env.SIGN_WIDEVINE_KEY || ''
  this.sign_widevine_passwd = process.env.SIGN_WIDEVINE_PASSPHRASE || ''
  this.signature_generator = path.join(this.srcDir, 'third_party', 'widevine', 'scripts', 'signature_generator.py') || ''
  this.extraGnArgs = {}
  this.extraGnGenOpts = getNPMConfig(['brave_extra_gn_gen_opts']) || ''
  this.extraNinjaOpts = []
  this.braveAndroidSafeBrowsingApiKey = getNPMConfig(['brave_safebrowsing_api_key']) || ''
  this.braveSafetyNetApiKey = getNPMConfig(['brave_safetynet_api_key']) || ''
  this.braveAndroidDeveloperOptionsCode = getNPMConfig(['brave_android_developer_options_code']) || ''
  this.braveAndroidKeystorePath = getNPMConfig(['brave_android_keystore_path'])
  this.braveAndroidKeystoreName = getNPMConfig(['brave_android_keystore_name'])
  this.braveAndroidKeystorePassword = getNPMConfig(['brave_android_keystore_password'])
  this.braveAndroidKeyPassword = getNPMConfig(['brave_android_key_password'])
  this.braveVariationsServerUrl = getNPMConfig(['brave_variations_server_url']) || ''
  this.nativeRedirectCCDir = path.join(this.srcDir, 'out', 'redirect_cc')
  this.use_goma = getNPMConfig(['brave_use_goma']) || false
  this.goma_offline = false
  this.use_libfuzzer = false

  if (process.env.GOMA_DIR !== undefined) {
    this.realGomaDir = process.env.GOMA_DIR
  } else {
    this.realGomaDir = path.join(this.depotToolsDir, '.cipd_bin')
  }
}

Config.prototype.isReleaseBuild = function () {
  return this.buildConfig === 'Release'
}

Config.prototype.isBraveReleaseBuild = function () {
  const npm_brave_relese_build = getNPMConfig(['is_brave_release_build'])
  if (npm_brave_relese_build !== undefined) {
    assert(npm_brave_relese_build === '0' || npm_brave_relese_build === '1',
      'Bad is_brave_release_build npm value (should be 0 or 1)')
    return npm_brave_relese_build === '1'
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
  const enable = this.buildConfig === 'Release' &&
    process.platform !== 'linux' &&
    this.sign_widevine_cert !== "" &&
    this.sign_widevine_key !== "" &&
    this.sign_widevine_passwd !== "" &&
    fs.existsSync(this.signature_generator)
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
  return this.isReleaseBuild() && !this.isAsan()
}

Config.prototype.getBrandingPathProduct = function () {
  return this.isOfficialBuild() ? "brave" : "brave-development"
}

Config.prototype.buildArgs = function () {
  const version = this.braveVersion
  let version_parts = version.split('+')[0]
  version_parts = version_parts.split('.')

  const chrome_version_parts = this.chromeVersion.split('.')

  let args = {
    sardine_client_id: this.sardineClientId,
    sardine_client_secret: this.sardineClientSecret,
    is_asan: this.isAsan(),
    enable_full_stack_frames_for_profiling: this.isAsan(),
    v8_enable_verify_heap: this.isAsan(),
    disable_fieldtrial_testing_config: true,
    safe_browsing_mode: 1,
    brave_services_key: this.braveServicesKey,
    root_extra_deps: ["//brave"],
    // TODO: Re-enable when chromium_src overrides work for files in relative
    // paths like widevine_cmdm_compoennt_installer.cc
    // use_jumbo_build: !this.officialBuild,
    is_component_build: this.isComponentBuild(),
    is_universal_binary: this.isUniversalBinary,
    proprietary_codecs: true,
    ffmpeg_branding: "Chrome",
    branding_path_component: "brave",
    branding_path_product: this.getBrandingPathProduct(),
    enable_nacl: false,
    enable_widevine: true,
    target_cpu: this.targetArch,
    is_official_build: this.isOfficialBuild(),
    is_debug: this.isDebug(),
    dcheck_always_on: getNPMConfig(['dcheck_always_on']) || this.isComponentBuild(),
    brave_channel: this.channel,
    brave_google_api_key: this.braveGoogleApiKey,
    brave_google_api_endpoint: this.googleApiEndpoint,
    google_default_client_id: this.googleDefaultClientId,
    google_default_client_secret: this.googleDefaultClientSecret,
    brave_infura_project_id: this.infuraProjectId,
    bitflyer_client_id: this.bitflyerClientId,
    bitflyer_client_secret: this.bitflyerClientSecret,
    bitflyer_staging_client_id: this.bitflyerStagingClientId,
    bitflyer_staging_client_secret: this.bitflyerStagingClientSecret,
    bitflyer_staging_url: this.bitflyerStagingUrl,
    gemini_api_url: this.geminiApiUrl,
    gemini_api_staging_url: this.geminiApiStagingUrl,
    gemini_oauth_url: this.geminiOauthUrl,
    gemini_oauth_staging_url: this.geminiOauthStagingUrl,
    gemini_wallet_client_id: this.geminiWalletClientId,
    gemini_wallet_client_secret: this.geminiWalletClientSecret,
    gemini_wallet_staging_client_id: this.geminiWalletStagingClientId,
    gemini_wallet_staging_client_secret: this.geminiWalletStagingClientSecret,
    uphold_client_id: this.upholdClientId,
    uphold_client_secret: this.upholdClientSecret,
    uphold_staging_client_id: this.upholdStagingClientId,
    uphold_staging_client_secret: this.upholdStagingClientSecret,
    brave_version_major: version_parts[0],
    brave_version_minor: version_parts[1],
    brave_version_build: version_parts[2],
    chrome_version_string: this.chromeVersion,
    brave_sync_endpoint: this.braveSyncEndpoint,
    safebrowsing_api_endpoint: this.safeBrowsingApiEndpoint,
    brave_variations_server_url: this.braveVariationsServerUrl,
    updater_prod_endpoint: this.updaterProdEndpoint,
    updater_dev_endpoint: this.updaterDevEndpoint,
    webcompat_report_api_endpoint: this.webcompatReportApiEndpoint,
    rewards_grant_dev_endpoint: this.rewardsGrantDevEndpoint,
    rewards_grant_staging_endpoint: this.rewardsGrantStagingEndpoint,
    rewards_grant_prod_endpoint: this.rewardsGrantProdEndpoint,
    brave_stats_api_key: this.braveStatsApiKey,
    brave_stats_updater_url: this.braveStatsUpdaterUrl,
    enable_hangout_services_extension: this.enable_hangout_services_extension,
    enable_cdm_host_verification: this.enableCDMHostVerification(),
    enable_pseudolocales: this.enable_pseudolocales,
    skip_signing: !this.shouldSign(),
    sparkle_dsa_private_key_file: this.sparkleDSAPrivateKeyFile,
    sparkle_eddsa_private_key: this.sparkleEdDSAPrivateKey,
    sparkle_eddsa_public_key: this.sparkleEdDSAPublicKey,
    use_goma: this.use_goma,
    use_libfuzzer: this.use_libfuzzer,
    enable_updater: this.isOfficialBuild(),
    enable_update_notifications: this.isOfficialBuild(),
    ...this.extraGnArgs,
  }

  if (!this.isBraveReleaseBuild()) {
    args.chrome_pgo_phase = 0

    if (process.platform === 'darwin' && this.targetOS != 'ios' && args.is_official_build) {
      // Currently we're using is_official_build mode in PR builds on CI. This enables dSYMs
      // by default, which slows down link phase, but also disables relocatable compilation
      // on MacOS (aka 'zero goma cachehits' style).
      //
      // Don't create dSYMs in non-public Release builds.
      // See //build/config/apple/symbols.gni for additional details.
      args.enable_dsyms = false
    }
  }

  if (this.shouldSign()) {
    if (process.platform === 'darwin') {
      args.mac_signing_identifier = this.mac_signing_identifier
      args.mac_installer_signing_identifier = this.mac_installer_signing_identifier
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
    }
  }

  if (process.platform === 'win32' && this.build_omaha) {
    args.build_omaha = this.build_omaha
    args.tag_ap = this.tag_ap
  }

  if ((process.platform === 'win32' || process.platform === 'darwin') && this.build_delta_installer) {
    assert(this.last_chrome_installer, 'Need last_chrome_installer args for building delta installer')
    args.build_delta_installer = true
    args.last_chrome_installer = this.last_chrome_installer
  }

  if (process.platform === 'darwin') {
    args.allow_runtime_configurable_key_storage = true
  }

  if (this.isDebug() &&
      !this.isComponentBuild() &&
      this.targetOS !== 'ios' &&
      this.targetOS !== 'android') {
    args.enable_profiling = true
  }

  if (this.sccache) {
    if (process.platform === 'win32') {
      args.clang_use_chrome_plugins = false
      args.use_thin_lto = true
    }
    args.enable_precompiled_headers = false
  }

  if (this.use_goma) {
    // set goma_dir to the redirect cc output dir which then calls gomacc
    // through env.CC_WRAPPER
    args.goma_dir = path.join(this.nativeRedirectCCDir)
  } else {
    args.cc_wrapper = path.join(this.nativeRedirectCCDir, 'redirect_cc')
  }

  if ((this.getTargetOS() === 'linux' && this.targetArch === 'x86') ||
      (this.getTargetOS() === 'win' && this.isBraveReleaseBuild())) {
    // Minimal symbols to work around size restrictions:
    // On Linux x86, ELF32 cannot be > 4GiB.
    // For x86, x64, and Arm64 Windows, chrome.dll.pdb sometimes becomes
    // > 4 GiB and llvm-pdbutil on that file errors out with "The data is in an
    // unexpected format. Too many directory blocks". Associated llvm issue:
    // https://github.com/llvm/llvm-project/issues/54445
    args.symbol_level = 1
  }

  if (this.getTargetOS() === 'mac' &&
      fs.existsSync(path.join(this.srcDir, 'build', 'mac_files', 'xcode_binaries', 'Contents'))) {
      // always use hermetic xcode for macos when available
      args.use_system_xcode = false
  }

  if (this.getTargetOS() === 'linux') {
    if (this.targetArch !== 'x86') {
      // Include vaapi support
      // TODO: Consider setting use_vaapi_x11 instead of use_vaapi. Also
      // consider enabling it for x86 builds. See
      // https://github.com/brave/brave-browser/issues/1024#issuecomment-1175397914
      args.use_vaapi = true

    }
    if (this.targetArch === 'arm64') {
      // We don't yet support Widevine on Arm64 Linux.
      args.enable_widevine = false
    }
  }

  // Enable Page Graph only in desktop builds.
  // Page Graph gn args should always be set explicitly, because they are parsed
  // from out/<dir>/args.gn by Python scripts during the build. We do this to
  // handle gn args in upstream build scripts without introducing git conflict.
  if (this.targetOS !== 'android' && this.targetOS !== 'ios') {
    args.enable_brave_page_graph = true
  } else {
    args.enable_brave_page_graph = false
  }
  // Enable Page Graph WebAPI probes only in dev/nightly builds.
  if (args.enable_brave_page_graph &&
      (!this.isBraveReleaseBuild() || this.channel === 'dev' ||
       this.channel === 'nightly')) {
    args.enable_brave_page_graph_webapi_probes = true
  } else {
    args.enable_brave_page_graph_webapi_probes = false
  }

  if (this.targetOS) {
    args.target_os = this.targetOS;
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
      args.exclude_unwind_tables = false
    } else if (this.channel === 'dev') {
      args.chrome_public_manifest_package = 'com.brave.browser_dev'
    } else if (this.channel === 'nightly') {
      args.android_channel = 'canary'
      args.chrome_public_manifest_package = 'com.brave.browser_nightly'
      args.exclude_unwind_tables = false
    }

    args.target_android_base = this.targetAndroidBase
    args.target_android_output_format =
      this.targetAndroidOutputFormat || (this.buildConfig === 'Release' ? 'aab' : 'apk')
    args.android_override_version_name = this.androidOverrideVersionName

    args.brave_android_developer_options_code = this.braveAndroidDeveloperOptionsCode
    args.brave_safetynet_api_key = this.braveSafetyNetApiKey
    args.brave_safebrowsing_api_key = this.braveAndroidSafeBrowsingApiKey
    args.enable_widevine = false
    args.safe_browsing_mode = 2

    // Feed is not used in Brave
    args.enable_feed_v2 = false

    // TODO(fixme)
    args.enable_tor = false

    // Fixes WebRTC IP leak with default option
    args.enable_mdns = true

    // We want it to be enabled for all configurations
    args.disable_android_lint = false

    if (this.targetArch === 'arm64') {
      // TODO: Ideally we should properly compile our rust libraries in order to
      // be able to use default 'standard' flow integrity. For now just revert
      // it to 'pac'.
      args.arm_control_flow_integrity = 'pac'
    }

    // These do not exist on android
    // TODO - recheck
    delete args.enable_nacl
    delete args.enable_hangout_services_extension
    // Ideally we'd not pass this on Linux CI and then
    // not have a default value for this. But we'll
    // eventually want it on Android, so keeping CI
    // unchanged and deleting here for now.
    delete args.gemini_client_secret
  }

  if (this.targetOS === 'ios') {
    if (this.targetEnvironment) {
      args.target_environment = this.targetEnvironment
    }
    if (this.targetArch == 'x64' && this.isDebug()) {
      args.use_lld = false
    }
    args.enable_dsyms = true
    args.enable_stripping = !this.isComponentBuild()
    // Component builds are not supported for iOS:
    // https://chromium.googlesource.com/chromium/src/+/master/docs/component_build.md
    args.is_component_build = false
    args.ios_enable_code_signing = false
    args.fatal_linker_warnings = !this.isComponentBuild()
    // DCHECK's crash on Static builds without allowing the debugger to continue
    // Can be removed when approprioate DCHECK's have been fixed:
    // https://github.com/brave/brave-browser/issues/10334
    args.dcheck_always_on = this.isComponentBuild()

    args.ios_enable_content_widget_extension = false
    args.ios_enable_search_widget_extension = false
    args.ios_enable_share_extension = false
    args.ios_enable_credential_provider_extension = false
    args.ios_enable_widget_kit_extension = false

    args.ios_provider_target = "//brave/ios/browser/providers:brave_providers"

    args.ios_locales_pack_extra_source_patterns = [
      "%root_gen_dir%/components/brave_components_strings_",
    ]
    args.ios_locales_pack_extra_deps = [
      "//brave/components/resources:strings",
    ]

    delete args.safebrowsing_api_endpoint
    delete args.safe_browsing_mode
    delete args.proprietary_codecs
    delete args.ffmpeg_branding
    delete args.branding_path_component
    delete args.branding_path_product
    delete args.enable_nacl
    delete args.enable_widevine
    delete args.enable_hangout_services_extension
    delete args.brave_google_api_endpoint
    delete args.brave_google_api_key
    delete args.brave_stats_api_key
    delete args.brave_stats_updater_url
    delete args.bitflyer_client_id
    delete args.bitflyer_client_secret
    delete args.bitflyer_staging_client_id
    delete args.bitflyer_staging_client_secret
    delete args.bitflyer_staging_url
    delete args.gemini_api_url
    delete args.gemini_api_staging_url
    delete args.gemini_oauth_url
    delete args.gemini_oauth_staging_url
    delete args.gemini_wallet_client_id
    delete args.gemini_wallet_client_secret
    delete args.gemini_wallet_staging_client_id
    delete args.gemini_wallet_staging_client_secret
    delete args.gemini_client_secret
    delete args.uphold_client_id
    delete args.uphold_client_secret
    delete args.uphold_staging_client_id
    delete args.uphold_staging_client_secret
    delete args.webcompat_report_api_endpoint
    delete args.use_blink_v8_binding_new_idl_interface
    delete args.v8_enable_verify_heap
    delete args.brave_variations_server_url
  }

  return args
}

Config.prototype.shouldSign = function () {
  if (this.skip_signing ||
    this.isComponentBuild() ||
    this.targetOS === 'ios') {
    return false
  }

  if (this.targetOS === 'android') {
    return this.braveAndroidKeystorePath !== undefined
  }

  if (process.platform === 'darwin') {
    return this.mac_signing_identifier !== undefined
  }

  if (process.platform === 'win32') {
    return process.env.CERT !== undefined ||
      process.env.AUTHENTICODE_HASH !== undefined ||
      process.env.SIGNTOOL_ARGS !== undefined
  }

  return false
}

Config.prototype.prependPath = function (oldPath, addPath) {
  let newPath = oldPath.split(path.delimiter)
  newPath.unshift(addPath)
  newPath = newPath.join(path.delimiter)
  return newPath
}

Config.prototype.appendPath = function (oldPath, addPath) {
  let newPath = oldPath.split(path.delimiter)
  newPath.push(addPath)
  newPath = newPath.join(path.delimiter)
  return newPath
}

Config.prototype.addPathToEnv = function (env, addPath, prepend = false) {
  // cmd.exe uses Path instead of PATH so just set both
  const addToPath = prepend ? this.prependPath : this.appendPath
  env.Path && (env.Path = addToPath(env.Path, addPath))
  env.PATH && (env.PATH = addToPath(env.PATH, addPath))
  return env
}

Config.prototype.addPythonPathToEnv = function (env, addPath) {
  env.PYTHONPATH = this.appendPath(env.PYTHONPATH || '', addPath)
  return env
}

Config.prototype.getProjectVersion = function (projectName) {
  return getNPMConfig(['projects', projectName, 'tag']) || getNPMConfig(['projects', projectName, 'branch'])
}

Config.prototype.getProjectRef = function (projectName) {
  const tag = getNPMConfig(['projects', projectName, 'tag'])
  if (tag) {
    return `refs/tags/${tag}`
  }

  let branch = getNPMConfig(['projects', projectName, 'branch'])
  if (branch) {
    return `origin/${branch}`
  }

  return 'origin/master'
}

Config.prototype.update = function (options) {
  if (options.sardine_client_secret) {
    this.sardineClientSecret = options.sardine_client_secret
  }

  if (options.sardine_client_id) {
    this.sardineClientId = options.sardine_client_id
  }

  if (options.universal) {
    this.targetArch = 'arm64'
    this.isUniversalBinary = true
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

  if (options.target_os === 'android') {
    this.targetOS = 'android'
    if (options.target_android_base) {
      this.targetAndroidBase = options.target_android_base
    }
    if (options.target_android_output_format) {
      this.targetAndroidOutputFormat = options.target_android_output_format
    }
    if (options.android_override_version_name) {
      this.androidOverrideVersionName = options.android_override_version_name
    }
  }

  if (options.target_os) {
    this.targetOS = options.target_os
  }

  if (options.target_environment) {
    this.targetEnvironment = options.target_environment
  }

  if (options.is_asan) {
    this.is_asan = true
  } else {
    this.is_asan = false
  }

  if (options.use_goma !== undefined) {
    this.use_goma = options.use_goma
  }

  if (options.goma_offline) {
    this.goma_offline = true
  }

  if (options.force_gn_gen) {
    this.force_gn_gen = true;
  } else {
    this.force_gn_gen = false;
  }

  if (options.C) {
    this.__outputDir = options.C
  }

  if (options.gclient_file && options.gclient_file !== 'default') {
    this.gClientFile = options.gclient_file
  }

  if (options.brave_google_api_key) {
    this.braveGoogleApiKey = options.brave_google_api_key
  }

  if (options.brave_safebrowsing_api_key) {
    this.braveAndroidSafeBrowsingApiKey = options.brave_safebrowsing_api_key
  }

  if (options.brave_safetynet_api_key) {
    this.braveSafetyNetApiKey = options.brave_safetynet_api_key
  }

  if (options.brave_google_api_endpoint) {
    this.googleApiEndpoint = options.brave_google_api_endpoint
  }

  if (options.brave_infura_project_id) {
    this.infuraProjectId = options.infura_project_id
  }

  if (options.bitflyer_client_id) {
    this.bitflyerClientId = options.bitflyer_client_id
  }

  if (options.bitflyer_client_secret) {
    this.bitflyerClientSecret = options.bitflyer_client_secret
  }

  if (options.bitflyer_staging_client_id) {
    this.bitflyerStagingClientId = options.bitflyer_staging_client_id
  }

  if (options.bitflyer_staging_client_secret) {
    this.bitflyerStagingClientSecret = options.bitflyer_staging_client_secret
  }

  if (options.bitflyer_staging_url) {
    this.bitflyerStagingUrl = options.bitflyer_staging_url
  }

  if (options.gemini_api_url) {
    this.geminiApiUrl = options.gemini_api_url
  }

  if (options.gemini_api_staging_url) {
    this.geminiApiStagingUrl = options.gemini_api_staging_url
  }

  if (options.gemini_oauth_url) {
    this.geminiOauthUrl = options.gemini_oauth_url
  }

  if (options.gemini_oauth_staging_url) {
    this.geminiOauthStagingUrl = options.gemini_oauth_staging_url
  }

  if (options.gemini_wallet_client_secret) {
    this.geminiWalletClientSecret = options.gemini_wallet_client_secret
  }

  if (options.gemini_wallet_staging_client_id) {
    this.geminiWalletStagingClientId = options.gemini_wallet_staging_client_id
  }

  if (options.gemini_wallet_staging_client_secret) {
    this.geminiWalletStagingClientSecret = options.gemini_wallet_staging_client_secret
  }

  if (options.uphold_client_id) {
    this.upholdClientId = options.uphold_client_id
  }

  if (options.uphold_client_secret) {
    this.upholdClientSecret = options.uphold_client_secret
  }

  if (options.uphold_staging_client_id) {
    this.upholdStagingClientId = options.uphold_staging_client_id
  }

  if (options.uphold_staging_client_secret) {
    this.upholdStagingClientSecret = options.uphold_staging_client_secret
  }

  if (options.safebrowsing_api_endpoint) {
    this.safeBrowsingApiEndpoint = options.safebrowsing_api_endpoint
  }

  if (options.updater_prod_endpoint) {
    this.updaterDevEndpoint = options.updater_prod_endpoint
  }

  if (options.updater_dev_endpoint) {
    this.updaterDevEndpoint = options.updater_dev_endpoint
  }

  if (options.webcompat_report_api_endpoint) {
    this.webcompatReportApiEndpoint = options.webcompat_report_api_endpoint
  }

  if (options.rewards_grant_dev_endpoint) {
    this.rewardsGrantDevEndpoint = options.rewards_grant_dev_endpoint
  }

  if (options.rewards_grant_staging_endpoint) {
    this.rewardsGrantStagingEndpoint = options.rewards_grant_staging_endpoint
  }

  if (options.rewards_grant_prod_endpoint) {
    this.rewardsGrantProdEndpoint = options.rewards_grant_prod_endpoint
  }

  if (options.brave_stats_api_key) {
    this.braveStatsApiKey = options.brave_stats_api_key
  }

  if (options.brave_stats_updater_url) {
    this.braveStatsUpdaterUrl = options.brave_stats_updater_url
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

  if (process.platform === 'win32' && options.build_omaha) {
    this.build_omaha = true
    this.tag_ap = options.tag_ap
  }

  if (options.skip_signing) {
    this.skip_signing = true
  }

  if (options.build_delta_installer) {
    this.build_delta_installer = true
    this.last_chrome_installer = options.last_chrome_installer
  }

  if (options.mac_signing_identifier)
    this.mac_signing_identifier = options.mac_signing_identifier

  if (options.mac_installer_signing_identifier)
    this.mac_installer_signing_identifier = options.mac_installer_signing_identifier

  if (options.mac_signing_keychain)
    this.mac_signing_keychain = options.mac_signing_keychain

  if (options.notarize)
    this.notarize = true

  if (options.gclient_verbose)
    this.gClientVerbose = options.gclient_verbose

  if (options.ignore_compile_failure)
    this.ignore_compile_failure = true

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
      opts.push(`-${key}`)
      opts.push(value)
    })
  }

  if (this.goma_offline || !this.use_goma) {
    // Pass '--offline' also when '--use_goma' is not set to disable goma detect in
    // autoninja when doing local builds.
    this.extraNinjaOpts.push('--offline')
  }

  if (options.target) {
    this.buildTarget = options.target
  }

  if (options.use_libfuzzer) {
    this.use_libfuzzer = options.use_libfuzzer
  }
}

Config.prototype.getTargetOS = function() {
  if (this.targetOS)
    return this.targetOS
  if (process.platform === 'darwin')
    return 'mac'
  if (process.platform === 'win32')
    return 'win'
  assert(process.platform === 'linux')
  return 'linux'
}

Config.prototype.getCachePath = function () {
  return this.git_cache_path || process.env.GIT_CACHE_PATH
}

Object.defineProperty(Config.prototype, 'defaultOptions', {
  get: function () {
    let env = Object.assign({}, process.env)
    env = this.addPathToEnv(env, path.join(this.depotToolsDir, 'python-bin'), true)
    env = this.addPathToEnv(env, path.join(this.depotToolsDir, 'python2-bin'), true)
    env = this.addPathToEnv(env, this.depotToolsDir, true)
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'brave', 'chromium_src', 'python_modules'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'brave', 'script'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'tools', 'grit', 'grit', 'extern'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'brave', 'vendor', 'requests'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'build'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'third_party', 'depot_tools'))
    env.DEPOT_TOOLS_WIN_TOOLCHAIN = '0'
    env.PYTHONUNBUFFERED = '1'
    env.TARGET_ARCH = this.gypTargetArch // for brave scripts
    env.GYP_MSVS_VERSION = env.GYP_MSVS_VERSION || '2017' // enable 2017

    // Fix `gclient runhooks` - broken since depot_tools a7b20b34f85432b5958963b75edcedfef9cf01fd
    env.GSUTIL_ENABLE_LUCI_AUTH = '0'

    if (this.channel != "") {
      env.BRAVE_CHANNEL = this.channel
    }

    if (this.getCachePath()) {
      console.log("using git cache path " + this.getCachePath())
      env.GIT_CACHE_PATH = path.join(this.getCachePath())
    }

    if (!this.use_goma && this.sccache) {
      env.CC_WRAPPER = this.sccache
      console.log('using cc wrapper ' + path.basename(this.sccache))
      if (path.basename(this.sccache) === 'ccache') {
        env.CCACHE_CPP2 = 'yes'
        env.CCACHE_SLOPPINESS = 'pch_defines,time_macros,include_file_mtime'
        env.CCACHE_BASEDIR = this.srcDir
        env = this.addPathToEnv(env, path.join(this.srcDir, 'third_party', 'llvm-build', 'Release+Asserts', 'bin'))
      }
    }

    if (this.use_goma) {
      // Vars used by autoninja to generate -j value, adjusted for Brave-specific setup.
      env.NINJA_CORE_MULTIPLIER = 20
      env.NINJA_CORE_LIMIT = 160
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

    return {
      env,
      stdio: 'inherit',
      cwd: this.srcDir,
      shell: true,
      git_cwd: '.',
    }
  },
})

Object.defineProperty(Config.prototype, 'outputDir', {
  get: function () {
    const baseDir = path.join(this.srcDir, 'out')
    if (this.__outputDir) {
      if (path.isAbsolute(this.__outputDir)) {
        return this.__outputDir;
      }
      return path.join(baseDir, this.__outputDir)
    }

    let buildConfigDir = this.buildConfig
    if (this.targetArch && this.targetArch != 'x64') {
      buildConfigDir = buildConfigDir + '_' + this.targetArch
    }
    if (this.targetOS && (this.targetOS === 'android' || this.targetOS === 'ios')) {
      buildConfigDir = this.targetOS + "_" + buildConfigDir
    }
    if (this.targetEnvironment) {
      buildConfigDir = buildConfigDir + "_" + this.targetEnvironment
    }

    return path.join(baseDir, buildConfigDir)
  },
  set: function (outputDir) { return this.__outputDir = outputDir },
})

module.exports = new Config
