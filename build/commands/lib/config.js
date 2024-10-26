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

const getEnvConfig = (key, default_value = undefined) => {
  if (!envConfig) {
    envConfig = {}

    // Parse src/brave/.env with all included env files.
    let envConfigPath = path.join(braveCoreDir, '.env')
    if (fs.existsSync(envConfigPath)) {
      dotenvPopulateWithIncludes(envConfig, envConfigPath)
    } else {
      // The .env file is used by `gn gen`. Create it if it doesn't exist.
      const defaultEnvConfigContent =
        '# This is a placeholder .env config file for the build system.\n' +
        '# See for details: https://github.com/brave/brave-browser/wiki/Build-configuration\n'
      fs.writeFileSync(envConfigPath, defaultEnvConfigContent)
    }

    // Convert 'true' and 'false' strings into booleans.
    for (const [key, value] of Object.entries(envConfig)) {
      if (value === 'true' || value === 'false') {
        envConfig[key] = value === 'true'
      }
    }
  }

  const envConfigValue = envConfig[key.join('_')]
  if (envConfigValue !== undefined)
    return envConfigValue

  const packageConfigValue = packageConfig(key)
  if (packageConfigValue !== undefined)
    return packageConfigValue

  return default_value
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
  assert(braveVersionParts.length == 3)
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
  this.internalDepsUrl = 'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.on.aws'
  this.defaultBuildConfig = getEnvConfig(['default_build_config']) || 'Component'
  this.buildConfig = this.defaultBuildConfig
  this.signTarget = 'sign_app'
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
  this.depotToolsRepo = getEnvConfig(['projects', 'depot_tools', 'repository', 'url'])
  this.defaultGClientFile = path.join(this.rootDir, '.gclient')
  this.gClientFile = process.env.BRAVE_GCLIENT_FILE || this.defaultGClientFile
  this.gClientVerbose = getEnvConfig(['gclient_verbose']) || false
  this.hostOS = getHostOS()
  this.targetArch = getEnvConfig(['target_arch']) || process.arch
  this.targetOS = getEnvConfig(['target_os'])
  this.targetEnvironment = getEnvConfig(['target_environment'])
  this.gypTargetArch = 'x64'
  this.targetAndroidBase = 'classic'
  this.braveServicesProductionDomain = getEnvConfig(['brave_services_production_domain']) || ''
  this.braveServicesStagingDomain = getEnvConfig(['brave_services_staging_domain']) || ''
  this.braveServicesDevDomain = getEnvConfig(['brave_services_dev_domain']) || ''
  this.braveGoogleApiKey = getEnvConfig(['brave_google_api_key']) || 'AIzaSyAREPLACEWITHYOUROWNGOOGLEAPIKEY2Q'
  this.googleApiEndpoint = getEnvConfig(['brave_google_api_endpoint']) || 'https://www.googleapis.com/geolocation/v1/geolocate?key='
  this.googleDefaultClientId = getEnvConfig(['google_default_client_id']) || ''
  this.googleDefaultClientSecret = getEnvConfig(['google_default_client_secret']) || ''
  this.infuraProjectId = getEnvConfig(['brave_infura_project_id']) || ''
  this.sardineClientId = getEnvConfig(['sardine_client_id']) || ''
  this.sardineClientSecret = getEnvConfig(['sardine_client_secret']) || ''
  this.bitFlyerProductionClientId = getEnvConfig(['bitflyer_production_client_id']) || ''
  this.bitFlyerProductionClientSecret = getEnvConfig(['bitflyer_production_client_secret']) || ''
  this.bitFlyerProductionFeeAddress = getEnvConfig(['bitflyer_production_fee_address']) || ''
  this.bitFlyerProductionUrl = getEnvConfig(['bitflyer_production_url']) || ''
  this.bitFlyerSandboxClientId = getEnvConfig(['bitflyer_sandbox_client_id']) || ''
  this.bitFlyerSandboxClientSecret = getEnvConfig(['bitflyer_sandbox_client_secret']) || ''
  this.bitFlyerSandboxFeeAddress = getEnvConfig(['bitflyer_sandbox_fee_address']) || ''
  this.bitFlyerSandboxUrl = getEnvConfig(['bitflyer_sandbox_url']) || ''
  this.geminiProductionApiUrl = getEnvConfig(['gemini_production_api_url']) || ''
  this.geminiProductionClientId = getEnvConfig(['gemini_production_client_id']) || ''
  this.geminiProductionClientSecret = getEnvConfig(['gemini_production_client_secret']) || ''
  this.geminiProductionFeeAddress = getEnvConfig(['gemini_production_fee_address']) || ''
  this.geminiProductionOauthUrl = getEnvConfig(['gemini_production_oauth_url']) || ''
  this.geminiSandboxApiUrl = getEnvConfig(['gemini_sandbox_api_url']) || ''
  this.geminiSandboxClientId = getEnvConfig(['gemini_sandbox_client_id']) || ''
  this.geminiSandboxClientSecret = getEnvConfig(['gemini_sandbox_client_secret']) || ''
  this.geminiSandboxFeeAddress = getEnvConfig(['gemini_sandbox_fee_address']) || ''
  this.geminiSandboxOauthUrl = getEnvConfig(['gemini_sandbox_oauth_url']) || ''
  this.upholdProductionApiUrl = getEnvConfig(['uphold_production_api_url']) || ''
  this.upholdProductionClientId = getEnvConfig(['uphold_production_client_id']) || ''
  this.upholdProductionClientSecret = getEnvConfig(['uphold_production_client_secret']) || ''
  this.upholdProductionFeeAddress = getEnvConfig(['uphold_production_fee_address']) || ''
  this.upholdProductionOauthUrl = getEnvConfig(['uphold_production_oauth_url']) || ''
  this.upholdSandboxApiUrl = getEnvConfig(['uphold_sandbox_api_url']) || ''
  this.upholdSandboxClientId = getEnvConfig(['uphold_sandbox_client_id']) || ''
  this.upholdSandboxClientSecret = getEnvConfig(['uphold_sandbox_client_secret']) || ''
  this.upholdSandboxFeeAddress = getEnvConfig(['uphold_sandbox_fee_address']) || ''
  this.upholdSandboxOauthUrl = getEnvConfig(['uphold_sandbox_oauth_url']) || ''
  this.zebPayProductionApiUrl = getEnvConfig(['zebpay_production_api_url']) || ''
  this.zebPayProductionClientId = getEnvConfig(['zebpay_production_client_id']) || ''
  this.zebPayProductionClientSecret = getEnvConfig(['zebpay_production_client_secret']) || ''
  this.zebPayProductionOauthUrl = getEnvConfig(['zebpay_production_oauth_url']) || ''
  this.zebPaySandboxApiUrl = getEnvConfig(['zebpay_sandbox_api_url']) || ''
  this.zebPaySandboxClientId = getEnvConfig(['zebpay_sandbox_client_id']) || ''
  this.zebPaySandboxClientSecret = getEnvConfig(['zebpay_sandbox_client_secret']) || ''
  this.zebPaySandboxOauthUrl = getEnvConfig(['zebpay_sandbox_oauth_url']) || ''
  this.braveSyncEndpoint = getEnvConfig(['brave_sync_endpoint']) || ''
  this.safeBrowsingApiEndpoint = getEnvConfig(['safebrowsing_api_endpoint']) || ''
  this.updaterProdEndpoint = getEnvConfig(['updater_prod_endpoint']) || ''
  this.updaterDevEndpoint = getEnvConfig(['updater_dev_endpoint']) || ''
  this.webcompatReportApiEndpoint = getEnvConfig(['webcompat_report_api_endpoint']) || 'https://webcompat.brave.com/1/webcompat'
  this.rewardsGrantDevEndpoint = getEnvConfig(['rewards_grant_dev_endpoint']) || ''
  this.rewardsGrantStagingEndpoint = getEnvConfig(['rewards_grant_staging_endpoint']) || ''
  this.rewardsGrantProdEndpoint = getEnvConfig(['rewards_grant_prod_endpoint']) || ''
  this.ignorePatchVersionNumber = !this.isBraveReleaseBuild() && getEnvConfig(['ignore_patch_version_number'], !this.isCI)
  this.braveVersion = getBraveVersion(this.ignorePatchVersionNumber)
  this.braveIOSMarketingPatchVersion = getEnvConfig(['brave_ios_marketing_version_patch']) || ''
  this.androidOverrideVersionName = this.braveVersion
  this.releaseTag = this.braveVersion.split('+')[0]
  this.mac_signing_identifier = getEnvConfig(['mac_signing_identifier'])
  this.mac_installer_signing_identifier = getEnvConfig(['mac_installer_signing_identifier']) || ''
  this.mac_signing_keychain = getEnvConfig(['mac_signing_keychain']) || 'login'
  this.sparkleDSAPrivateKeyFile = getEnvConfig(['sparkle_dsa_private_key_file']) || ''
  this.sparkleEdDSAPrivateKey = getEnvConfig(['sparkle_eddsa_private_key']) || ''
  this.sparkleEdDSAPublicKey = getEnvConfig(['sparkle_eddsa_public_key']) || ''
  this.notary_user = getEnvConfig(['notary_user']) || ''
  this.notary_password = getEnvConfig(['notary_password']) || ''
  this.channel = 'development'
  this.git_cache_path = getEnvConfig(['git_cache_path'])
  this.sccache = getEnvConfig(['sccache'])
  this.rbeService = getEnvConfig(['rbe_service']) || ''
  this.rbeTlsClientAuthCert = getEnvConfig(['rbe_tls_client_auth_cert']) || ''
  this.rbeTlsClientAuthKey = getEnvConfig(['rbe_tls_client_auth_key']) || ''
  // Make sure "src/" is a part of RBE "exec_root" to allow "src/" files as inputs.
  this.rbeExecRoot = this.rootDir
  this.realRewrapperDir = process.env.RBE_DIR || path.join(this.srcDir, 'buildtools', 'reclient')
  this.braveStatsApiKey = getEnvConfig(['brave_stats_api_key']) || ''
  this.braveStatsUpdaterUrl = getEnvConfig(['brave_stats_updater_url']) || ''
  this.ignore_compile_failure = false
  this.enable_hangout_services_extension = false
  this.enable_pseudolocales = false
  this.sign_widevine_cert = process.env.SIGN_WIDEVINE_CERT || ''
  this.sign_widevine_key = process.env.SIGN_WIDEVINE_KEY || ''
  this.sign_widevine_passwd = process.env.SIGN_WIDEVINE_PASSPHRASE || ''
  this.signature_generator = path.join(this.srcDir, 'third_party', 'widevine', 'scripts', 'signature_generator.py') || ''
  this.extraGnArgs = {}
  this.extraGnGenOpts = getEnvConfig(['brave_extra_gn_gen_opts']) || ''
  this.extraNinjaOpts = []
  this.braveAndroidSafeBrowsingApiKey = getEnvConfig(['brave_safebrowsing_api_key']) || ''
  this.braveSafetyNetApiKey = getEnvConfig(['brave_safetynet_api_key']) || ''
  this.braveAndroidDeveloperOptionsCode = getEnvConfig(['brave_android_developer_options_code']) || ''
  this.braveAndroidKeystorePath = getEnvConfig(['brave_android_keystore_path'])
  this.braveAndroidKeystoreName = getEnvConfig(['brave_android_keystore_name'])
  this.braveAndroidKeystorePassword = getEnvConfig(['brave_android_keystore_password'])
  this.braveAndroidKeyPassword = getEnvConfig(['brave_android_key_password'])
  this.braveVariationsServerUrl = getEnvConfig(['brave_variations_server_url']) || ''
  this.nativeRedirectCCDir = path.join(this.srcDir, 'out', 'redirect_cc')
  this.useRemoteExec = getEnvConfig(['use_remoteexec']) || false
  this.offline = getEnvConfig(['offline']) || false
  this.use_libfuzzer = false
  this.androidAabToApk = false
  this.useBraveHermeticToolchain = this.rbeService.includes('.brave.com:')
  this.brave_services_key_id = getEnvConfig(['brave_services_key_id']) || ''
  this.service_key_aichat = getEnvConfig(['service_key_aichat']) || ''
  this.braveIOSDeveloperOptionsCode = getEnvConfig(['brave_ios_developer_options_code']) || ''
  this.service_key_stt = getEnvConfig(['service_key_stt']) || ''
}

Config.prototype.isReleaseBuild = function () {
  return this.buildConfig === 'Release'
}

Config.prototype.isBraveReleaseBuild = function () {
  const isBraveReleaseBuildValue = getEnvConfig(['is_brave_release_build'])
  if (isBraveReleaseBuildValue !== undefined) {
    assert(isBraveReleaseBuildValue === '0' || isBraveReleaseBuildValue === '1',
      'Bad is_brave_release_build value (should be 0 or 1)')
    return isBraveReleaseBuildValue === '1'
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

Config.prototype.getBraveLogoIconName = function () {
  let iconName = "brave-icon-dev-color.svg"
  if (this.isBraveReleaseBuild()) {
    if (this.channel === "beta") {
      iconName = "brave-icon-beta-color.svg"
    } else if (this.channel === "nightly") {
      iconName = "brave-icon-nightly-color.svg"
    } else {
      iconName = "brave-icon-release-color.svg"
    }
  }
  return iconName
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
    root_extra_deps: ["//brave"],
    clang_unsafe_buffers_paths: "//brave/build/config/unsafe_buffers_paths.txt",
    // TODO: Re-enable when chromium_src overrides work for files in relative
    // paths like widevine_cmdm_compoennt_installer.cc
    // use_jumbo_build: !this.officialBuild,
    is_component_build: this.isComponentBuild(),
    is_universal_binary: this.isUniversalBinary,
    proprietary_codecs: true,
    ffmpeg_branding: "Chrome",
    branding_path_component: "brave",
    branding_path_product: "brave",
    enable_nacl: false,
    enable_widevine: true,
    // Our copy of signature_generator.py doesn't support --ignore_missing_cert:
    ignore_missing_widevine_signing_cert: false,
    target_cpu: this.targetArch,
    is_official_build: this.isOfficialBuild(),
    is_debug: this.isDebug(),
    dcheck_always_on: getEnvConfig(['dcheck_always_on']) || this.isComponentBuild(),
    brave_channel: this.channel,
    brave_google_api_key: this.braveGoogleApiKey,
    brave_google_api_endpoint: this.googleApiEndpoint,
    google_default_client_id: this.googleDefaultClientId,
    google_default_client_secret: this.googleDefaultClientSecret,
    brave_infura_project_id: this.infuraProjectId,
    bitflyer_production_client_id: this.bitFlyerProductionClientId,
    bitflyer_production_client_secret: this.bitFlyerProductionClientSecret,
    bitflyer_production_fee_address: this.bitFlyerProductionFeeAddress,
    bitflyer_production_url: this.bitFlyerProductionUrl,
    bitflyer_sandbox_client_id: this.bitFlyerSandboxClientId,
    bitflyer_sandbox_client_secret: this.bitFlyerSandboxClientSecret,
    bitflyer_sandbox_fee_address: this.bitFlyerSandboxFeeAddress,
    bitflyer_sandbox_url: this.bitFlyerSandboxUrl,
    gemini_production_api_url: this.geminiProductionApiUrl,
    gemini_production_client_id: this.geminiProductionClientId,
    gemini_production_client_secret: this.geminiProductionClientSecret,
    gemini_production_fee_address: this.geminiProductionFeeAddress,
    gemini_production_oauth_url: this.geminiProductionOauthUrl,
    gemini_sandbox_api_url: this.geminiSandboxApiUrl,
    gemini_sandbox_client_id: this.geminiSandboxClientId,
    gemini_sandbox_client_secret: this.geminiSandboxClientSecret,
    gemini_sandbox_fee_address: this.geminiSandboxFeeAddress,
    gemini_sandbox_oauth_url: this.geminiSandboxOauthUrl,
    uphold_production_api_url: this.upholdProductionApiUrl,
    uphold_production_client_id: this.upholdProductionClientId,
    uphold_production_client_secret: this.upholdProductionClientSecret,
    uphold_production_fee_address: this.upholdProductionFeeAddress,
    uphold_production_oauth_url: this.upholdProductionOauthUrl,
    uphold_sandbox_api_url: this.upholdSandboxApiUrl,
    uphold_sandbox_client_id: this.upholdSandboxClientId,
    uphold_sandbox_client_secret: this.upholdSandboxClientSecret,
    uphold_sandbox_fee_address: this.upholdSandboxFeeAddress,
    uphold_sandbox_oauth_url: this.upholdSandboxOauthUrl,
    zebpay_production_api_url: this.zebPayProductionApiUrl,
    zebpay_production_client_id: this.zebPayProductionClientId,
    zebpay_production_client_secret: this.zebPayProductionClientSecret,
    zebpay_production_oauth_url: this.zebPayProductionOauthUrl,
    zebpay_sandbox_api_url: this.zebPaySandboxApiUrl,
    zebpay_sandbox_client_id: this.zebPaySandboxClientId,
    zebpay_sandbox_client_secret: this.zebPaySandboxClientSecret,
    zebpay_sandbox_oauth_url: this.zebPaySandboxOauthUrl,
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
    use_remoteexec: this.useRemoteExec,
    use_libfuzzer: this.use_libfuzzer,
    enable_updater: this.isOfficialBuild(),
    enable_update_notifications: this.isOfficialBuild(),
    brave_services_production_domain: this.braveServicesProductionDomain,
    brave_services_staging_domain: this.braveServicesStagingDomain,
    brave_services_dev_domain: this.braveServicesDevDomain,
    brave_services_key_id: this.brave_services_key_id,
    service_key_aichat: this.service_key_aichat,
    service_key_stt: this.service_key_stt,
    generate_about_credits: true,
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
    if (this.getTargetOS() === 'mac') {
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

  if (this.build_omaha) {
    args.build_omaha = this.build_omaha
    args.tag_ap = this.tag_ap
    if (this.tag_installdataindex) {
      args.tag_installdataindex = this.tag_installdataindex
    }
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

  if (this.useRemoteExec) {
    args.rbe_exec_root = this.rbeExecRoot
    args.reclient_bin_dir = path.join(this.nativeRedirectCCDir)
  } else {
    args.cc_wrapper = path.join(this.nativeRedirectCCDir, 'redirect_cc')
  }

  // Adjust symbol_level in Linux builds:
  // 1. Set minimal symbol level to workaround size restrictions: on Linux x86,
  //    ELF32 cannot be > 4GiB.
  // 2. Enable symbols in Static builds. By default symbol_level is 0 in this
  //    configuration. symbol_level = 2 cannot be used because of "relocation
  //    R_X86_64_32 out of range" errors.
  if (
    this.getTargetOS() === 'linux' &&
    (this.targetArch === 'x86' ||
      (!this.isDebug() && !this.isComponentBuild() && !this.isReleaseBuild()))
  ) {
    args.symbol_level = 1
  }

  // For Linux Release builds, upstream doesn't want to use symbol_level = 2
  // unless use_debug_fission is set. However, they don't set it when a
  // cc_wrapper is used. Since we use cc_wrapper we need to set it manually.
  if (this.getTargetOS() === 'linux' && this.isReleaseBuild()) {
    args.use_debug_fission = true
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
  }

  if (['android', 'linux', 'mac'].includes(this.getTargetOS())) {
    // LSAN only works with ASAN and has very low overhead.
    args.is_lsan = args.is_asan
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

  // Devtools: Now we patch devtools frontend, so it is useful to see
  // if something goes wrong on CI builds.
  if (this.targetOS !== 'android' && this.targetOS !== 'ios' && this.isCI) {
    args.devtools_skip_typecheck = false
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
      this.targetAndroidOutputFormat || (this.buildConfig === 'Release' ? 'aab' : 'apk')
    args.android_override_version_name = this.androidOverrideVersionName

    args.brave_android_developer_options_code = this.braveAndroidDeveloperOptionsCode
    args.brave_safetynet_api_key = this.braveSafetyNetApiKey
    args.brave_safebrowsing_api_key = this.braveAndroidSafeBrowsingApiKey
    args.safe_browsing_mode = 2

    // Required since cr126 to use Chrome password store
    args.use_login_database_as_backend = true

    // TODO(fixme)
    args.enable_tor = false

    // Fixes WebRTC IP leak with default option
    args.enable_mdns = true

    // We want it to be enabled for all configurations
    args.disable_android_lint = false

    args.android_aab_to_apk = this.androidAabToApk

    if (this.targetArch == "arm64") {
      // Flag use_relr_relocations is incompatible with Android 8 arm64, but
      // makes huge optimizations on Android 9 and above.
      // Decision is to specify android:minSdkVersion=28 for arm64 and keep
      // 26(default) for arm32.
      // Then:
      //   - for Android 8 and 8.1 GP will supply arm32 bundle;
      //   - for Android 9 and above GP will supply arm64 and we can enable all
      //     optimizations.
      args.default_min_sdk_version = 28
    }

    // These do not exist on android
    // TODO - recheck
    delete args.enable_nacl
    delete args.enable_hangout_services_extension
  }

  if (this.targetOS === 'ios') {
    if (this.targetEnvironment) {
      args.target_environment = this.targetEnvironment
    }
    if (this.braveIOSMarketingPatchVersion != '') {
      args.brave_ios_marketing_version_patch = this.braveIOSMarketingPatchVersion
    }
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

    if (!args.is_official_build) {
      // When building locally iOS needs dSYMs in order for Xcode to map source
      // files correctly since we are using a framework build
      args.enable_dsyms = true
      if (args.use_remoteexec) {
        // RBE expects relative paths in dSYMs
        args.strip_absolute_paths_from_debug_symbols = true
      }
    }

    args.ios_enable_content_widget_extension = false
    args.ios_enable_search_widget_extension = false
    args.ios_enable_share_extension = false
    args.ios_enable_credential_provider_extension = true
    args.ios_enable_widget_kit_extension = false

    args.brave_ios_developer_options_code = this.braveIOSDeveloperOptionsCode

    // This is currently being flipped on and off by the Chromium team to test
    // however it causes crashes for us at launch. Check `ios/features.gni`
    // in the future to see if this is no longer needed
    // https://github.com/brave/brave-browser/issues/29934
    args.ios_partition_alloc_enabled = false
    args.use_partition_alloc = false

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
  if (this.skip_signing ||
    this.isComponentBuild() ||
    this.targetOS === 'ios') {
    return false
  }

  if (this.targetOS === 'android') {
    return this.braveAndroidKeystorePath !== undefined
  }

  if (this.getTargetOS() === 'mac') {
    return this.mac_signing_identifier !== undefined
  }

  if (process.platform === 'win32') {
    return process.env.CERT !== undefined ||
      process.env.AUTHENTICODE_HASH !== undefined ||
      process.env.SIGNTOOL_ARGS !== undefined
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
    getEnvConfig(['projects', projectName, 'revision']) ||
    getEnvConfig(['projects', projectName, 'tag']) ||
    getEnvConfig(['projects', projectName, 'branch'])
  )
}

Config.prototype.getProjectRef = function (projectName, defaultValue = 'origin/master') {
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

  if (options.target_os) {
    // Handle non-standard target_os values as they are used on CI currently and
    // it's easier to support them as is instead of rewriting the CI scripts.
    if (options.target_os === 'macos') {
      this.targetOS = 'mac';
    } else if (options.target_os === 'windows') {
      this.targetOS = 'win';
    } else {
      this.targetOS = options.target_os;
    }
    assert(
      ['android', 'ios', 'linux', 'mac', 'win'].includes(this.targetOS),
      `Unsupported target_os value: ${this.targetOS}`
    )
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

  if (options.target_environment) {
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

  if (options.use_remoteexec !== undefined) {
    this.useRemoteExec = options.use_remoteexec
  }

  if (options.offline) {
    this.offline = true
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
    assert(options.tag_ap, "--tag_ap is required for --build_omaha")
  }

  if (options.tag_ap) {
    assert(options.build_omaha, "--tag_ap requires --build_omaha")
    this.tag_ap = options.tag_ap
  }

  if (options.tag_installdataindex) {
    assert(options.build_omaha, "--tag_installdataindex requires --build_omaha")
    this.tag_installdataindex = options.tag_installdataindex
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
}

Config.prototype.getTargetOS = function() {
  if (this.targetOS)
    return this.targetOS
  return this.hostOS
}

Config.prototype.getCachePath = function () {
  return this.git_cache_path || process.env.GIT_CACHE_PATH
}

Object.defineProperty(Config.prototype, 'defaultOptions', {
  get: function () {
    let env = Object.assign({}, process.env)
    env = this.addPathToEnv(env, path.join(this.depotToolsDir, 'python-bin'),
                            true)
    env = this.addPathToEnv(env, path.join(this.depotToolsDir, 'python2-bin'),
                            true)
    env = this.addPathToEnv(env, path.join(this.srcDir, 'third_party',
                                           'rust-toolchain', 'bin'), true)
    env = this.addPathToEnv(env, this.depotToolsDir, true)
    if (this.getTargetOS() === 'mac' && process.platform !== 'darwin') {
      const crossCompilePath = path.join(this.srcDir, 'brave', 'build', 'mac',
                                         'cross-compile', 'path')
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
    pythonPaths.forEach(p => {
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

    if (this.channel != "") {
      env.BRAVE_CHANNEL = this.channel
    }

    if (!this.useBraveHermeticToolchain) {
      env.DEPOT_TOOLS_WIN_TOOLCHAIN = '0'
    } else {
      // Use hermetic toolchain only internally.
      env.USE_BRAVE_HERMETIC_TOOLCHAIN = '1'
      env.DEPOT_TOOLS_WIN_TOOLCHAIN = '1'
      env.GYP_MSVS_HASH_7393122652 = 'd325744cf9'
      env.DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL = `${this.internalDepsUrl}/windows-hermetic-toolchain/`
    }

    if (this.getCachePath()) {
      env.GIT_CACHE_PATH = path.join(this.getCachePath())
    }

    if (!this.useRemoteExec && this.sccache) {
      env.CC_WRAPPER = this.sccache
      console.log('using cc wrapper ' + path.basename(this.sccache))
      if (path.basename(this.sccache) === 'ccache') {
        env.CCACHE_CPP2 = 'yes'
        env.CCACHE_SLOPPINESS = 'pch_defines,time_macros,include_file_mtime'
        env.CCACHE_BASEDIR = this.srcDir
        env = this.addPathToEnv(env, path.join(this.srcDir, 'third_party', 'llvm-build', 'Release+Asserts', 'bin'))
      }
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

    if (this.useRemoteExec) {
      // These env vars are required during `build` stage.

      // Autoninja generates -j value when RBE is enabled, adjust limits for
      // Brave-specific setup.
      env.NINJA_CORE_MULTIPLIER = Math.min(20, env.NINJA_CORE_MULTIPLIER || 20)
      env.NINJA_CORE_LIMIT = Math.min(160, env.NINJA_CORE_LIMIT || 160)

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

    // TeamCity displays only stderr on the "Build Problems" page when an error
    // occurs. By redirecting stdout to stderr, we ensure that all outputs from
    // external processes are visible in case of a failure.
    const stdio = this.isTeamcity ? ['inherit', process.stderr, 'inherit'] : 'inherit'

    return {
      env,
      stdio: stdio,
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
    if (this.targetOS && this.targetOS !== this.hostOS) {
      buildConfigDir = this.targetOS + '_' + buildConfigDir
    }
    if (this.targetEnvironment) {
      buildConfigDir = buildConfigDir + "_" + this.targetEnvironment
    }
    if (this.isChromium) {
      buildConfigDir = buildConfigDir + "_chromium"
    }

    return path.join(baseDir, buildConfigDir)
  },
  set: function (outputDir) { return this.__outputDir = outputDir },
})

module.exports = new Config
