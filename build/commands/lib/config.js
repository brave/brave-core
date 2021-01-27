'use strict'

const path = require('path')
const fs = require('fs')
const assert = require('assert')
const { spawnSync } = require('child_process')
const rootDir = require('./root')

let npmCommand = 'npm'
if (process.platform === 'win32') {
  npmCommand += '.cmd'
}
let NpmConfig = null

const run = (cmd, args = []) => {
  const prog = spawnSync(cmd, args)
  if (prog.status !== 0) {
    console.log(prog.stdout && prog.stdout.toString())
    console.error(prog.stderr && prog.stderr.toString())
    process.exit(1)
  }
  return prog
}

// this is a huge hack because the npm config doesn't get passed through from brave-browser .npmrc/package.json
var packageConfig = function(key){
  let packages = { config: {}}
  if (fs.existsSync(path.join(rootDir, 'package.json'))) {
    packages = require(path.relative(__dirname, path.join(rootDir, 'package.json')))
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

const getNPMConfig = (key) => {
  if (!NpmConfig) {
    const list = run(npmCommand, ['config', 'list', '--json', '--userconfig=' + path.join(rootDir, '.npmrc')])
    NpmConfig = JSON.parse(list.stdout.toString())
  }

  return NpmConfig[key.join('-').replace(/_/g, '-')] || packageConfig(key)
}

const parseExtraInputs = (inputs, accumulator, callback) => {
  for (let input of inputs) {
    let separatorIndex = input.indexOf(':')
    if (separatorIndex < 0) {
      separatorIndex = input.length
    }

    const key = input.substring(0, separatorIndex);
    const value = input.substring(separatorIndex + 1);
    callback(accumulator, key, value)
  }
}

const Config = function () {
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
  this.braveCoreDir = path.join(this.srcDir, 'brave')
  this.braveCoreRepo = getNPMConfig(['projects', 'brave-core', 'repository', 'url'])
  this.buildToolsDir = path.join(this.srcDir, 'build')
  this.resourcesDir = path.join(this.rootDir, 'resources')
  this.depotToolsDir = path.join(this.braveCoreDir, 'vendor', 'depot_tools')
  this.defaultGClientFile = path.join(this.rootDir, '.gclient')
  this.gClientFile = process.env.BRAVE_GCLIENT_FILE || this.defaultGClientFile
  this.gClientVerbose = getNPMConfig(['gclient_verbose']) || false
  this.targetArch = getNPMConfig(['target_arch']) || 'x64'
  this.targetOS = getNPMConfig(['target_os'])
  this.gypTargetArch = 'x64'
  this.targetApkBase ='classic'
  this.braveGoogleApiKey = getNPMConfig(['brave_google_api_key']) || 'AIzaSyAQfxPJiounkhOjODEO5ZieffeBv6yft2Q'
  this.googleApiEndpoint = getNPMConfig(['brave_google_api_endpoint']) || 'https://www.googleapis.com/geolocation/v1/geolocate?key='
  this.googleDefaultClientId = getNPMConfig(['google_default_client_id']) || ''
  this.googleDefaultClientSecret = getNPMConfig(['google_default_client_secret']) || ''
  this.braveServicesKey = getNPMConfig(['brave_services_key']) || ''
  this.infuraProjectId = getNPMConfig(['brave_infura_project_id']) || ''
  this.binanceClientId = getNPMConfig(['binance_client_id']) || ''
  this.bitflyerClientSecret = getNPMConfig(['bitflyer_client_secret']) || ''
  this.bitflyerStagingUrl = getNPMConfig(['bitflyer_staging_url']) || ''
  this.geminiClientId = getNPMConfig(['gemini_client_id']) || ''
  this.geminiClientSecret = getNPMConfig(['gemini_client_secret']) || ''
  this.braveSyncEndpoint = getNPMConfig(['brave_sync_endpoint']) || ''
  this.safeBrowsingApiEndpoint = getNPMConfig(['safebrowsing_api_endpoint']) || ''
  this.updaterProdEndpoint = getNPMConfig(['updater_prod_endpoint']) || ''
  this.updaterDevEndpoint = getNPMConfig(['updater_dev_endpoint']) || ''
  this.webcompatReportApiEndpoint = getNPMConfig(['webcompat_report_api_endpoint']) || 'https://webcompat.brave.com/1/webcompat'
  this.chromePgoPhase = 0
  // this.buildProjects()
  this.braveVersion = getNPMConfig(['version']) || '0.0.0'
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
  this.braveStatsApiKey = getNPMConfig(['brave_stats_api_key']) || ''
  this.braveStatsUpdaterUrl = getNPMConfig(['brave_stats_updater_url']) || ''
  this.ignore_compile_failure = false
  this.enable_hangout_services_extension = true
  this.sign_widevine_cert = process.env.SIGN_WIDEVINE_CERT || ''
  this.sign_widevine_key = process.env.SIGN_WIDEVINE_KEY || ''
  this.sign_widevine_passwd = process.env.SIGN_WIDEVINE_PASSPHRASE || ''
  this.signature_generator = path.join(this.srcDir, 'third_party', 'widevine', 'scripts', 'signature_generator.py') || ''
  this.extraGnArgs = {}
  this.extraNinjaOpts = []
  this.braveSafetyNetApiKey = getNPMConfig(['brave_safetynet_api_key']) || ''
  this.braveAndroidDeveloperOptionsCode = getNPMConfig(['brave_android_developer_options_code']) || ''
  this.braveAndroidKeystorePath = getNPMConfig(['brave_android_keystore_path'])
  this.braveAndroidKeystoreName = getNPMConfig(['brave_android_keystore_name'])
  this.braveAndroidKeystorePassword = getNPMConfig(['brave_android_keystore_password'])
  this.braveAndroidKeyPassword = getNPMConfig(['brave_android_key_password'])
  this.braveVariationsServerUrl = getNPMConfig(['brave_variations_server_url']) || ''
}

Config.prototype.isOfficialBuild = function () {
  return this.buildConfig === 'Release'
}

Config.prototype.isComponentBuild = function () {
  return this.buildConfig === 'Debug' || this.buildConfig === 'Component'
}

Config.prototype.isDebug = function () {
  return this.buildConfig === 'Debug'
}

Config.prototype.isDcheckAlwaysOn = function () {
  return this.buildConfig !== 'Release'
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

Config.prototype.buildArgs = function () {
  const version = this.braveVersion
  let version_parts = version.split('+')[0]
  version_parts = version_parts.split('.')

  const chrome_version_parts = this.chromeVersion.split('.')

  let args = {
    is_asan: this.isAsan(),
    enable_full_stack_frames_for_profiling: this.isAsan(),
    v8_enable_verify_heap: this.isAsan(),
    fieldtrial_testing_like_official_build: true,
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
    enable_nacl: false,
    enable_widevine: true,
    target_cpu: this.targetArch,
    is_official_build: this.isOfficialBuild() && !this.isAsan(),
    is_debug: this.isDebug(),
    dcheck_always_on: this.isDcheckAlwaysOn(),
    brave_channel: this.channel,
    brave_google_api_key: this.braveGoogleApiKey,
    brave_google_api_endpoint: this.googleApiEndpoint,
    google_default_client_id: this.googleDefaultClientId,
    google_default_client_secret: this.googleDefaultClientSecret,
    brave_infura_project_id: this.infuraProjectId,
    binance_client_id: this.binanceClientId,
    bitflyer_client_secret: this.bitflyerClientSecret,
    bitflyer_staging_url: this.bitflyerStagingUrl,
    gemini_client_id: this.geminiClientId,
    gemini_client_secret: this.geminiClientSecret,
    brave_product_name: getNPMConfig(['brave_product_name']) || "brave-core",
    brave_project_name: getNPMConfig(['brave_project_name']) || "brave-core",
    brave_version_major: version_parts[0],
    brave_version_minor: version_parts[1],
    brave_version_build: version_parts[2],
    chrome_version_string: this.chromeVersion,
    chrome_version_major: chrome_version_parts[0],
    brave_sync_endpoint: this.braveSyncEndpoint,
    safebrowsing_api_endpoint: this.safeBrowsingApiEndpoint,
    brave_variations_server_url: this.braveVariationsServerUrl,
    updater_prod_endpoint: this.updaterProdEndpoint,
    updater_dev_endpoint: this.updaterDevEndpoint,
    webcompat_report_api_endpoint: this.webcompatReportApiEndpoint,
    brave_stats_api_key: this.braveStatsApiKey,
    brave_stats_updater_url: this.braveStatsUpdaterUrl,
    enable_hangout_services_extension: this.enable_hangout_services_extension,
    enable_cdm_host_verification: this.enableCDMHostVerification(),
    skip_signing: !this.shouldSign(),
    chrome_pgo_phase: this.chromePgoPhase,
    sparkle_dsa_private_key_file: this.sparkleDSAPrivateKeyFile,
    sparkle_eddsa_private_key: this.sparkleEdDSAPrivateKey,
    sparkle_eddsa_public_key: this.sparkleEdDSAPublicKey,
    ...this.extraGnArgs,
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

  if (this.isDebug() &&
      this.targetOS !== 'ios' &&
      this.targetOS !== 'android') {
    if (process.platform === 'darwin') {
      args.enable_stripping = false
    }
    args.symbol_level = 2
    args.enable_profiling = true
  }

  if (this.sccache && process.platform === 'win32') {
    args.clang_use_chrome_plugins = false
    args.enable_precompiled_headers = false
    args.use_thin_lto = true
  }

  if (this.targetArch === 'x86' && process.platform === 'linux') {
    // Minimal symbols for target Linux x86, because ELF32 cannot be > 4GiB
    args.symbol_level = 1
  }

  if (this.targetArch === 'x64' &&
      process.platform === 'linux' &&
      this.targetOS !== 'android') {
    // Include vaapi support
    args.use_vaapi = true
  }

  if (this.targetOS === 'android') {
    args.target_os = 'android'
    args.android_channel = this.channel
    if (!this.isOfficialBuild()) {
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

    args.target_apk_base = this.targetApkBase
    args.android_override_version_name = this.androidOverrideVersionName

    args.brave_android_developer_options_code = this.braveAndroidDeveloperOptionsCode
    args.brave_safetynet_api_key = this.braveSafetyNetApiKey
    args.enable_widevine = false
    args.safe_browsing_mode = 2

    if (this.buildConfig !== 'Release') {
      // treat non-release builds like Debug builds
      args.treat_warnings_as_errors = false
      // TODO(samartnik): component builds crash at the moment on Android.
      // We will need to revert this change once this fix is landed in upstream
      // https://bugs.chromium.org/p/chromium/issues/detail?id=1166748#c14
      args.is_component_build = false
    } else {
      // otherwise there is build error
      // ld.lld: error: output file too large: 5861255936 bytes
      args.symbol_level = 1
    }

    // Feed is not used in Brave
    args.enable_feed_v1 = false
    args.enable_feed_v2 = false

    // TODO(fixme)
    args.enable_tor = false

    // These do not exist on android
    // TODO - recheck
    delete args.enable_nacl
    delete args.enable_hangout_services_extension
    delete args.brave_infura_project_id
    // Ideally we'd not pass this on Linux CI and then
    // not have a default value for this. But we'll
    // eventually want it on Android, so keeping CI
    // unchanged and deleting here for now.
    delete args.gemini_client_id
    delete args.gemini_client_secret
  }

  if (this.targetOS === 'ios') {
    args.target_os = 'ios'
    args.enable_dsyms = true
    args.enable_stripping = !this.isDebug()
    args.use_xcode_clang = false
    args.use_clang_coverage = false
    // Component builds are not supported for iOS:
    // https://chromium.googlesource.com/chromium/src/+/master/docs/component_build.md
    args.is_component_build = false
    args.ios_deployment_target = '12.0'
    args.ios_enable_code_signing = false
    args.fatal_linker_warnings = !this.isComponentBuild()
    // DCHECK's crash on Static builds without allowing the debugger to continue
    // Can be removed when approprioate DCHECK's have been fixed:
    // https://github.com/brave/brave-browser/issues/10334
    args.dcheck_always_on = this.isDebug()

    args.ios_enable_content_widget_extension = false
    args.ios_enable_search_widget_extension = false
    args.ios_enable_share_extension = false
    args.ios_enable_credential_provider_extension = false
    args.ios_enable_widget_kit_extension = false

    delete args.safebrowsing_api_endpoint
    delete args.updater_prod_endpoint
    delete args.updater_dev_endpoint
    delete args.safe_browsing_mode
    delete args.proprietary_codecs
    delete args.ffmpeg_branding
    delete args.branding_path_component
    delete args.enable_nacl
    delete args.branding_path_component
    delete args.enable_widevine
    delete args.enable_hangout_services_extension
    delete args.brave_google_api_endpoint
    delete args.brave_google_api_key
    delete args.brave_stats_api_key
    delete args.brave_stats_updater_url
    delete args.brave_infura_project_id
    delete args.binance_client_id
    delete args.bitflyer_client_secret
    delete args.bitflyer_staging_url
    delete args.gemini_client_id
    delete args.gemini_client_secret
    delete args.webcompat_report_api_endpoint
    delete args.use_blink_v8_binding_new_idl_interface
    delete args.v8_enable_verify_heap
    delete args.brave_variations_server_url
  }

  if (process.platform === 'win32') {
    args.cc_wrapper = path.join(this.srcDir, 'brave', 'script', 'redirect-cc.cmd')
  } else {
    args.cc_wrapper = path.join(this.srcDir, 'brave', 'script', 'redirect-cc.py')
  }
  return args
}

Config.prototype.shouldSign = function () {
  if (this.skip_signing ||
      this.buildConfig !== 'Release' ||
      this.targetOS === 'ios') {
    return false;
  }

  if (this.targetOS === 'android') {
    return this.braveAndroidKeystorePath !== undefined
  }

  if (process.platform === 'darwin') {
    return this.mac_signing_identifier !== undefined
  }

  if (process.platform === 'win32') {
    return process.env.CERT !== undefined &&
           process.env.SIGNTOOL_ARGS !== undefined
  }

  return false;
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
    if (options.target_apk_base) {
      this.targetApkBase = options.target_apk_base
    }
    if (options.android_override_version_name) {
      this.androidOverrideVersionName = options.android_override_version_name
    }
  }

  if (options.target_os) {
    this.targetOS = options.target_os
  }

  if (options.is_asan) {
    this.is_asan = true
  } else {
    this.is_asan = false
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

  if (options.brave_safetynet_api_key) {
    this.braveSafetyNetApiKey = options.brave_safetynet_api_key;
  }

  if (options.brave_google_api_endpoint) {
    this.googleApiEndpoint = options.brave_google_api_endpoint
  }

  if (options.brave_infura_project_id) {
    this.infuraProjectId = options.infura_project_id
  }

  if (options.binance_client_id) {
    this.binanceClientId = options.binance_client_id
  }

  if (options.bitflyer_client_secret) {
    this.bitflyerClientSecret = options.bitflyer_client_secret
  }

  if (options.bitflyer_staging_url) {
    this.bitflyerStagingUrl = options.bitflyer_staging_url
  }

  if (options.gemini_client_id) {
    this.geminiClientId = options.gemini_client_id
  }

  if (options.gemini_client_secret) {
    this.geminiClientSecret = options.gemini_client_secret
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
      this.xcode_gen_target = '//brave/vendor/brave-ios:*'
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
}

Config.prototype.getCachePath = function () {
  return this.git_cache_path || process.env.GIT_CACHE_PATH
}

Object.defineProperty(Config.prototype, 'defaultOptions', {
  get: function () {
    let env = Object.assign({}, process.env)
    env = this.addPathToEnv(env, this.depotToolsDir, true)
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'brave', 'chromium_src', 'python_modules'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'brave', 'script'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'tools', 'grit', 'grit', 'extern'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'brave', 'vendor', 'requests'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'build'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'third_party', 'depot_tools'))
    env.GCLIENT_FILE = this.gClientFile
    env.DEPOT_TOOLS_WIN_TOOLCHAIN = '0'
    env.PYTHONUNBUFFERED = '1'
    env.TARGET_ARCH = this.gypTargetArch // for brave scripts
    env.GYP_MSVS_VERSION = env.GYP_MSVS_VERSION || '2017' // enable 2017
    if (this.channel != "") {
      env.BRAVE_CHANNEL = this.channel
    }

    if (this.getCachePath()) {
      console.log("using git cache path " + this.getCachePath())
      env.GIT_CACHE_PATH = path.join(this.getCachePath())
    }

    if (this.sccache) {
      env.CC_WRAPPER = this.sccache
      if (path.basename(this.sccache) === 'ccache') {
        console.log('using ccache')
        env.CCACHE_CPP2 = 'yes'
        env.CCACHE_SLOPPINESS = 'pch_defines,time_macros,include_file_mtime'
        env.CCACHE_BASEDIR = this.srcDir
        env = this.addPathToEnv(env, path.join(this.srcDir, 'third_party', 'llvm-build', 'Release+Asserts', 'bin'))
      } else {
        console.log('using sccache')
      }
    }

    if (process.platform === 'linux') {
      env.LLVM_DOWNLOAD_GOLD_PLUGIN = '1'
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
    if (this.__outputDir)
      return path.join(baseDir, this.__outputDir)
    let buildConfigDir = this.buildConfig
    if (this.targetArch && this.targetArch != 'x64') {
      buildConfigDir = buildConfigDir + '_' + this.targetArch
    }
    if (this.targetOS) {
      buildConfigDir = this.targetOS + "_" + buildConfigDir
    }

    return path.join(baseDir, buildConfigDir)
  },
  set: function (outputDir) { return this.__outputDir = outputDir },
})

module.exports = new Config
