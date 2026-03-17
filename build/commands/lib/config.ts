// Copyright (c) 2016 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

import os from 'node:os'
import path from 'node:path'
import fs from 'node:fs'
import assert from 'node:assert'
import rootDir from './rootDir.cjs'
import EnvConfig from './envConfig.ts'
import * as Log from './log.ts'
import util from './util.js'
import { isCI, isTeamcity } from './ciDetect.ts'

type ExecOptions = {
  env: NodeJS.ProcessEnv
  stdio: any
  cwd: string
  git_cwd: string
  continueOnFail?: boolean
  onStdOutLine?: (line: string) => void
  onStdErrLine?: (line: string) => void
}

type TargetOS = 'android' | 'ios' | 'linux' | 'mac' | 'win'

const braveCoreDir = path.join(rootDir, 'src', 'brave')

const envConfig = new EnvConfig(braveCoreDir)

export class Config {
  internalDepsUrl: string
  defaultBuildConfig: string
  buildConfig: string
  buildTargets: string[]
  rootDir: string
  isUniversalBinary: boolean = false
  isChromium: boolean = false
  scriptDir: string
  srcDir: string
  chromeVersion: string | undefined
  chromiumRepo: string
  braveCoreDir: string
  buildToolsDir: string
  resourcesDir: string
  depotToolsDir: string
  depotToolsRepo: string
  gclientFile: string
  gclientVerbose: boolean
  disableGclientConfigUpdate: boolean
  gclientGlobalVars: Record<string, any>
  targetArch: string
  targetEnvironment: string | undefined
  gypTargetArch: string
  ignorePatchVersionNumber: boolean
  useDummyLastchange: boolean
  braveVersion: string
  braveIOSMarketingPatchVersion: string | undefined
  androidOverrideVersionName: string
  releaseTag: string | undefined
  mac_signing_identifier: string | undefined
  mac_installer_signing_identifier: string
  mac_signing_keychain: string
  notary_user: string | undefined
  notary_password: string | undefined
  channel: string
  isBraveOriginBranded: boolean | undefined
  gitCachePath: string | undefined
  rbeService: string
  rbeTlsClientAuthCert: string | undefined
  rbeTlsClientAuthKey: string | undefined
  realRewrapperDir: string
  ignore_compile_failure: boolean
  enable_hangout_services_extension: boolean
  sign_widevine_cert: string
  sign_widevine_key: string
  sign_widevine_passwd: string
  signature_generator: string
  extraGnArgs: Record<string, any>
  extraGnGenOpts: string
  extraNinjaOpts: string[]
  sisoJobsLimit: number | undefined
  sisoCacheDir: string | undefined
  braveAndroidSafeBrowsingApiKey: string | undefined
  braveAndroidDeveloperOptionsCode: string | undefined
  braveAndroidKeystorePath: string | undefined
  braveAndroidKeystoreName: string | undefined
  braveAndroidKeystorePassword: string | undefined
  braveAndroidKeyPassword: string | undefined
  braveAndroidPkcs11Provider: string
  braveAndroidPkcs11Alias: string
  nativeRedirectCCDir: string
  useRemoteExec: boolean
  useSiso: boolean
  useReclient: boolean
  offline: boolean
  use_libfuzzer: boolean
  androidAabToApk: boolean
  useBraveHermeticToolchain: boolean
  braveIOSDeveloperOptionsCode: string | undefined
  skip_download_rust_toolchain_aux: boolean
  is_asan: boolean | undefined
  is_msan: boolean | undefined
  is_ubsan: boolean | undefined
  use_no_gn_gen: boolean | undefined

  chromiumCustomDeps: Record<string, any>
  chromiumCustomVars: Record<string, any>

  skip_signing: boolean | undefined
  targetAndroidOutputFormat: string | undefined
  use_clang_coverage: boolean | undefined
  force_gn_gen: boolean | undefined
  build_omaha: boolean | undefined
  tag_ap: string | undefined
  tag_installdataindex: string | undefined
  last_chrome_installer: string | undefined
  notarize: boolean | undefined
  xcode_gen_target: string | undefined

  constructor() {
    this.internalDepsUrl =
      'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.on.aws'
    this.defaultBuildConfig = envConfig.getString(
      ['default_build_config'],
      'Component',
    )
    this.buildConfig = this.defaultBuildConfig
    this.buildTargets = ['brave']
    this.rootDir = rootDir
    this.isUniversalBinary = false
    this.isChromium = false
    this.scriptDir = path.join(this.rootDir, 'scripts')
    this.srcDir = path.join(this.rootDir, 'src')
    this.chromeVersion = this.getProjectVersion('chrome')
    this.chromiumRepo = envConfig.requireString([
      'projects',
      'chrome',
      'repository',
      'url',
    ])
    this.braveCoreDir = braveCoreDir
    this.buildToolsDir = path.join(this.srcDir, 'build')
    this.resourcesDir = path.join(this.rootDir, 'resources')
    this.depotToolsDir = envConfig.requirePath([
      'projects',
      'depot_tools',
      'dir',
    ])
    this.depotToolsRepo = envConfig.requireString([
      'projects',
      'depot_tools',
      'repository',
      'url',
    ])
    this.gclientFile = path.join(this.rootDir, '.gclient')
    this.gclientVerbose = envConfig.getBoolean(['gclient_verbose'], false)
    this.disableGclientConfigUpdate = envConfig.getBoolean(
      ['disable_gclient_config_update'],
      false,
    )
    this.gclientGlobalVars = envConfig.getMergedObject([
      'gclient',
      'global_vars',
    ])
    this.targetOS = envConfig.getString(['target_os'], this.hostOS)
    this.targetArch = envConfig.getString(['target_arch'], process.arch)
    this.targetEnvironment = envConfig.getString(['target_environment'])
    this.gypTargetArch = 'x64'
    this.ignorePatchVersionNumber =
      !this.isBraveReleaseBuild()
      && envConfig.getBoolean(['ignore_patch_version_number'], !isCI)
    this.useDummyLastchange = envConfig.getBoolean(
      ['use_dummy_lastchange'],
      true,
    )
    this.braveVersion = this.#getBraveVersion()
    this.braveIOSMarketingPatchVersion = envConfig.getString(
      ['brave_ios_marketing_version_patch'],
      '',
    )
    this.androidOverrideVersionName = this.braveVersion
    this.releaseTag = this.braveVersion.split('+')[0]
    this.mac_signing_identifier = envConfig.getString([
      'mac_signing_identifier',
    ])
    this.mac_installer_signing_identifier = envConfig.getString(
      ['mac_installer_signing_identifier'],
      '',
    )
    this.mac_signing_keychain = envConfig.getString(
      ['mac_signing_keychain'],
      'login',
    )
    this.notary_user = envConfig.getString(['notary_user'])
    this.notary_password = envConfig.getString(['notary_password'])
    this.channel = 'development'
    this.isBraveOriginBranded = envConfig.getBoolean([
      'is_brave_origin_branded',
    ])
    this.gitCachePath =
      envConfig.getPath(['git_cache_path']) || process.env.GIT_CACHE_PATH
    this.rbeService = envConfig.getString(['rbe_service'], '')
    this.rbeTlsClientAuthCert = envConfig.getPath(['rbe_tls_client_auth_cert'])
    this.rbeTlsClientAuthKey = envConfig.getPath(['rbe_tls_client_auth_key'])
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
    this.extraGnGenOpts = envConfig.getString(['brave_extra_gn_gen_opts'], '')
    this.extraNinjaOpts = []
    this.sisoJobsLimit = undefined
    this.sisoCacheDir = envConfig.getPath(['siso_cache_dir'])
    this.braveAndroidSafeBrowsingApiKey = envConfig.getString([
      'brave_safebrowsing_api_key',
    ])
    this.braveAndroidDeveloperOptionsCode = envConfig.getString([
      'brave_android_developer_options_code',
    ])
    this.braveAndroidKeystorePath = envConfig.getString([
      'brave_android_keystore_path',
    ])
    this.braveAndroidKeystoreName = envConfig.getString([
      'brave_android_keystore_name',
    ])
    this.braveAndroidKeystorePassword = envConfig.getString([
      'brave_android_keystore_password',
    ])
    this.braveAndroidKeyPassword = envConfig.getString([
      'brave_android_key_password',
    ])
    this.braveAndroidPkcs11Provider = ''
    this.braveAndroidPkcs11Alias = ''
    this.nativeRedirectCCDir = path.join(this.srcDir, 'out', 'redirect_cc')
    this.useRemoteExec = envConfig.getBoolean(['use_remoteexec'], false)
    this.useSiso = envConfig.getBoolean(['use_siso'], true)
    this.useReclient = envConfig.getBoolean(
      ['use_reclient'],
      this.useRemoteExec && !this.useSiso,
    )
    this.offline = envConfig.getBoolean(['offline'], false)
    this.use_libfuzzer = false
    this.androidAabToApk = false
    this.useBraveHermeticToolchain = envConfig.getBoolean(
      ['use_brave_hermetic_toolchain'],
      this.rbeService.includes('.brave.com:'),
    )
    this.braveIOSDeveloperOptionsCode = envConfig.getString([
      'brave_ios_developer_options_code',
    ])
    this.skip_download_rust_toolchain_aux = envConfig.getBoolean(
      ['skip_download_rust_toolchain_aux'],
      false,
    )
    this.is_asan = envConfig.getBoolean(['is_asan'])
    this.is_msan = envConfig.getBoolean(['is_msan'])
    this.is_ubsan = envConfig.getBoolean(['is_ubsan'])
    this.use_no_gn_gen = envConfig.getBoolean(['use_no_gn_gen'])

    this.chromiumCustomDeps = envConfig.getMergedObject([
      'projects',
      'chrome',
      'custom_deps',
    ])
    this.chromiumCustomVars = {
      'checkout_pgo_profiles': this.isBraveReleaseBuild(),
      ...(this.rbeService
        ? {
            'reapi_address': this.rbeService,
            'reapi_backend_config_path': 'google.star',
            'reapi_instance': 'default',
          }
        : {}),
      ...envConfig.getMergedObject(['projects', 'chrome', 'custom_vars']),
    }
  }

  isReleaseBuild() {
    return this.buildConfig === 'Release'
  }

  isBraveReleaseBuild() {
    const isBraveReleaseBuildValue = envConfig.getNumber([
      'is_brave_release_build',
    ])
    if (isBraveReleaseBuildValue !== undefined) {
      assert(
        isBraveReleaseBuildValue === 0 || isBraveReleaseBuildValue === 1,
        'Bad is_brave_release_build value (should be 0 or 1)',
      )
      return isBraveReleaseBuildValue === 1
    }

    return false
  }

  isComponentBuild() {
    return this.buildConfig === 'Debug' || this.buildConfig === 'Component'
  }

  isDebug() {
    return this.buildConfig === 'Debug'
  }

  enableCDMHostVerification() {
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

  isAsan() {
    if (this.is_asan !== undefined) {
      return this.is_asan
    }
    return false
  }

  isLsan() {
    // LSAN only works with ASAN and has very low overhead.
    // Chromium supports LeakSanitizer is supported on x86_64 Linux only.
    // See https://www.chromium.org/developers/testing/leaksanitizer/
    return this.isAsan() && this.targetOS === 'linux'
  }

  isOfficialBuild() {
    return (
      this.isReleaseBuild() && !this.isAsan() && !this.is_msan && !this.is_ubsan
    )
  }

  getBraveLogoIconName() {
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

  shouldSign() {
    if (
      this.skip_signing
      || this.isComponentBuild()
      || this.targetOS === 'ios'
    ) {
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

  addToPath(oldPath, addPath, prepend = false) {
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

  addPathToEnv(env, addPath, prepend = false) {
    // cmd.exe uses Path instead of PATH so just set both
    env.Path && (env.Path = this.addToPath(env.Path, addPath, prepend))
    env.PATH && (env.PATH = this.addToPath(env.PATH, addPath, prepend))
    return env
  }

  addPythonPathToEnv(env, addPath) {
    env.PYTHONPATH = this.addToPath(env.PYTHONPATH, addPath)
    return env
  }

  getProjectVersion(projectName) {
    return (
      envConfig.getString(['projects', projectName, 'revision'])
      || envConfig.getString(['projects', projectName, 'tag'])
      || envConfig.getString(['projects', projectName, 'branch'])
    )
  }

  getProjectRef(projectName, defaultValue = 'origin/master') {
    const revision = envConfig.getString(['projects', projectName, 'revision'])
    if (revision) {
      return revision
    }

    const tag = envConfig.getString(['projects', projectName, 'tag'])
    if (tag) {
      return `refs/tags/${tag}`
    }

    let branch = envConfig.getString(['projects', projectName, 'branch'])
    if (branch) {
      return `origin/${branch}`
    }

    return defaultValue
  }

  updateInternal(options) {
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
      this.#outputDir = options.C
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
      assert(
        options.build_omaha,
        '--tag_installdataindex requires --build_omaha',
      )
      this.tag_installdataindex = options.tag_installdataindex
    }

    if (options.skip_signing) {
      this.skip_signing = true
    }

    if (options.last_chrome_installer) {
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
      this.gclientVerbose = options.gclient_verbose
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
      parseExtraInputs(
        options.ninja,
        this.extraNinjaOpts,
        (opts, key, value) => {
          // Workaround siso unable to handle -j if REAPI is not configured.
          if (key === 'j' && this.useSiso) {
            this.sisoJobsLimit = parseInt(value)
            return
          }
          opts.push(`-${key}`)
          if (value) {
            opts.push(value)
          }
        },
      )
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

  fromGnArgs(options) {
    const gnArgs = readArgsGn(this.srcDir, options.C)
    Log.warn(
      '--no-gn-gen is experimental and only gn args that match command '
        + 'line options will be processed',
    )
    this.updateInternal(Object.assign({}, gnArgs, options))
    assert(!isCI)
  }

  update(options) {
    if (this.use_no_gn_gen) {
      this.fromGnArgs(options)
    } else {
      this.updateInternal(options)
    }
  }

  isIOS() {
    return this.targetOS === 'ios'
  }

  isAndroid() {
    return this.targetOS === 'android'
  }

  isMobile() {
    return this.isIOS() || this.isAndroid()
  }

  forwardEnvConfigVarsToObject(vars, obj) {
    for (const v of vars) {
      obj[v] = envConfig.getAny([v])
    }
  }

  #getBraveVersion() {
    const braveVersion = envConfig.getPackageVersion()
    if (!this.ignorePatchVersionNumber) {
      return braveVersion
    }

    const braveVersionParts = braveVersion.split('.')
    assert(braveVersionParts.length === 3)
    braveVersionParts[2] = '0'
    return braveVersionParts.join('.')
  }

  get defaultOptions(): ExecOptions {
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
      env.GYP_MSVS_HASH_e66617bc68 = 'e66617bc68'
      env.DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL = `${this.internalDepsUrl}/windows-hermetic-toolchain/`
    }

    if (this.rbeService) {
      // These env vars are required during `sync` stage.
      env.RBE_service = env.RBE_service || this.rbeService
      if (this.rbeTlsClientAuthCert && this.rbeTlsClientAuthKey) {
        env.RBE_tls_client_auth_cert =
          env.RBE_tls_client_auth_cert || this.rbeTlsClientAuthCert
        env.RBE_tls_client_auth_key =
          env.RBE_tls_client_auth_key || this.rbeTlsClientAuthKey
        env.RBE_service_no_auth = env.RBE_service_no_auth || 'true'
        env.RBE_use_application_default_credentials =
          env.RBE_use_application_default_credentials || 'true'
      }
    }

    // These env vars are required during `build` stage.
    if (this.useRemoteExec) {
      // Restrict remote execution jobs to x1.2 of available executors. This is
      // a slight overprovisioning to ensure that network latency does not cause
      // remote execution starvation. Apply siso hard limit to avoid overloading
      // low-CPU machines.
      const kExecutorCount = 1200
      const kRemoteLimit = Math.min(
        envConfig.getNumber(['rbe_jobs_limit'], kExecutorCount * 1.2),
        getSisoBuiltinRemoteLimit(),
      )

      // Prevent depot_tools from setting lower timeouts.
      const kRbeTimeout = '10m'
      env.RBE_exec_timeout = env.RBE_exec_timeout || kRbeTimeout
      env.RBE_reclient_timeout = env.RBE_reclient_timeout || kRbeTimeout

      // Autoninja generates -j value when RBE is enabled, adjust limits for
      // Brave-specific setup.
      env.NINJA_CORE_MULTIPLIER = Math.min(
        20,
        // @ts-ignore
        parseInt(env.NINJA_CORE_MULTIPLIER) || 20,
      ).toString()
      env.NINJA_CORE_LIMIT = Math.min(
        kRemoteLimit,
        // @ts-ignore
        parseInt(env.NINJA_CORE_LIMIT) || kRemoteLimit,
      ).toString()

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
        (env.SISO_LIMITS?.split(',').map((item) => item.split('=', 2)) as [
          string,
          string,
        ][]) || [],
      )
      // Merge defaultSisoLimits with envSisoLimits ensuring that the values are
      // not greater than the default values.
      Object.entries(defaultSisoLimits).forEach(([key, defaultValue]) => {
        if (defaultValue === undefined) {
          return
        }
        // @ts-ignore
        const valueFromEnv = parseInt(envSisoLimits.get(key)) || defaultValue
        envSisoLimits.set(key, Math.min(defaultValue, valueFromEnv).toString())
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

    if (isCI) {
      // Enables autoninja to show build speed and final stats on finish.
      env.NINJA_SUMMARIZE_BUILD = '1'
    }

    if (process.platform === 'linux') {
      env.LLVM_DOWNLOAD_GOLD_PLUGIN = '1'
    }

    if (process.platform === 'win32') {
      // Disable vcvarsall.bat telemetry.
      env.VSCMD_SKIP_SENDTELEMETRY = '1'
    }

    if (isCI && this.skip_download_rust_toolchain_aux) {
      env.SKIP_DOWNLOAD_RUST_TOOLCHAIN_AUX = '1'
    }

    // TeamCity displays only stderr on the "Build Problems" page when an error
    // occurs. By redirecting stdout to stderr, we ensure that all outputs from
    // external processes are visible in case of a failure.
    const stdio = isTeamcity
      ? ['inherit', process.stderr, 'inherit']
      : 'inherit'

    return {
      env,
      stdio,
      cwd: this.srcDir,
      git_cwd: '.',
    }
  }

  #outputDir: string | undefined

  get outputDir() {
    if (this.use_no_gn_gen && this.#outputDir === undefined) {
      Log.error(`You must specify output directory with -C with use_no_gn_gen`)
      process.exit(1)
    }

    const baseDir = path.join(this.srcDir, 'out')
    if (this.#outputDir) {
      if (path.isAbsolute(this.#outputDir)) {
        return this.#outputDir
      }
      return path.join(baseDir, this.#outputDir)
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
  }

  set outputDir(outputDir) {
    this.#outputDir = outputDir
  }

  get hostOS() {
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

  #targetOS: TargetOS | undefined

  get targetOS() {
    return this.#targetOS ?? this.hostOS
  }

  set targetOS(value: string) {
    const supportedTargetOS = ['android', 'ios', 'linux', 'mac', 'win']
    if (!supportedTargetOS.includes(value)) {
      Log.error(
        `Invalid target_os: ${value} (must be one of: ${supportedTargetOS.join(', ')})`,
      )
      process.exit(1)
    }
    this.#targetOS = value as TargetOS
  }
}

function readArgsGn(srcDir, outputDir) {
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

  const result = util.run(
    'python3',
    ['-'],
    util.mergeWithDefault({
      skipLogging: true,
      stdio: 'pipe',
      input: script,
      encoding: 'utf8',
    }),
  )

  return JSON.parse(result.stdout.toString().trim())
}

function parseExtraInputs(inputs, accumulator, callback) {
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

// Mirrors limitForRemote() from siso source to apply a hard limit.
// https://source.chromium.org/chromium/build/+/main:siso/build/limits.go;l=169-181;drc=c2c13435ffe51d890a46d488c48dee362f82453b
function getSisoBuiltinRemoteLimit() {
  const kRemoteLimitFactor = 80
  const kReproxyLimitCap = 5000
  const limit = kRemoteLimitFactor * os.cpus().length
  if (process.platform === 'darwin' && process.arch === 'x64') {
    return Math.min(1000, limit)
  }
  return Math.min(kReproxyLimitCap, limit)
}

export default new Config()
