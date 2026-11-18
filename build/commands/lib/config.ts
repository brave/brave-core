// Copyright (c) 2016 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import os from 'node:os'
import path from 'node:path'
import fs from 'node:fs'
import assert from 'node:assert'
import rootDir from './rootDir.cjs'
import EnvConfig from './envConfig.ts'
import * as Log from './log.ts'
import util from './util.ts'
import { isCI, isTeamcity } from './ciDetect.ts'
import type * as buildOptions from './buildOptions.ts'
import type { Builder } from './builderTypes.ts'

type ExecOptions = {
  env: NodeJS.ProcessEnv
  stdio: any
  cwd: string
  continueOnFail?: boolean
  onStdOutLine?: (line: string) => void
  onStdErrLine?: (line: string) => void
}

type UpdateOptions = buildOptions.BuildDirOptions
  & buildOptions.TargetConfigOptions
  & buildOptions.GnArgsOptions
  & buildOptions.GnGenOptions
  & buildOptions.NinjaOptions & {
    build_config?: string | undefined
    gclient_verbose?: boolean | undefined
    lean_sync?: boolean | undefined
  }

const validTargetOSValues = ['android', 'ios', 'linux', 'mac', 'win'] as const
type TargetOS = (typeof validTargetOSValues)[number]

const braveCoreDir = path.join(rootDir, 'src', 'brave')

// TODO(http://brave.dev/b/56449): Experimental implementation for integration
// with our bots generator.
// The only .env keys read with `--builder`, which takes everything else from
// the builder: what stays local to the machine, its remote execution, caches
// and output. Keep this short and audited.
const BUILDER_ALLOWED_DOTENV_KEYS: readonly string[] = [
  'cache_dir',
  'gclient_verbose',
  'git_cache_path',
  'lean_sync',
  'offline',
  'quiet',
  'rbe_jobs_limit',
  'rbe_readonly',
  'rbe_service',
  'rbe_tls_client_auth_cert',
  'rbe_tls_client_auth_key',
  'siso_cache_dir',
  'use_remoteexec',
]

export class Config {
  internalDepsUrl: string
  buildTargets: string[]
  rootDir: string
  isUniversalBinary: boolean = false
  isChromium: boolean = false
  scriptDir: string
  srcDir: string
  braveCoreDir: string
  buildToolsDir: string
  resourcesDir: string
  gclientFile: string
  gerritMirrorsUser: string | undefined
  gerritMirrorGitConfig: string
  gypTargetArch: string
  channel: string
  ignore_compile_failure: boolean
  enable_hangout_services_extension: boolean
  sign_widevine_cert: string
  sign_widevine_key: string
  sign_widevine_passwd: string
  signature_generator: string
  extraNinjaOpts: string[]
  sisoJobsLimit: number | undefined
  braveAndroidPkcs11Provider: string
  braveAndroidPkcs11Alias: string
  use_libfuzzer: boolean
  androidAabToApk: boolean

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

  #envConfig: EnvConfig
  // TODO(http://brave.dev/b/56449): Experimental implementation for integration
  // with our bots generator.
  // The `--builder` the config is for, which provides what .env otherwise
  // does: the builder's values, and fixed ones for what a builder does not
  // configure.
  #builder: Builder | undefined

  // The values of the properties that can be set, once read or set.
  #values = new Map<keyof Config, unknown>()

  // The checkout's config, read from its package.json and `dotenvConfig`, or
  // its .env files without it. With a builder, .env may only have
  // BUILDER_ALLOWED_DOTENV_KEYS, and reading any other value package.json does not
  // provide throws.
  static load(builder?: Builder, dotenvConfig?: NodeJS.Dict<string>): Config {
    return new Config(
      new EnvConfig(
        braveCoreDir,
        dotenvConfig,
        builder ? BUILDER_ALLOWED_DOTENV_KEYS : undefined,
      ),
      builder,
    )
  }

  constructor(envConfig: EnvConfig, builder?: Builder) {
    if (builder && !builder.spec.chromium_config.build_config) {
      throw new Error(`builder ${builder.name}: its spec has no build_config`)
    }
    this.#envConfig = envConfig
    this.#builder = builder
    this.internalDepsUrl =
      'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.on.aws'
    this.buildTargets = ['brave']
    this.rootDir = rootDir
    this.isUniversalBinary = false
    this.isChromium = false
    this.scriptDir = path.join(this.rootDir, 'scripts')
    this.srcDir = path.join(this.rootDir, 'src')
    this.braveCoreDir = braveCoreDir
    this.buildToolsDir = path.join(this.srcDir, 'build')
    this.resourcesDir = path.join(this.rootDir, 'resources')
    this.gclientFile = path.join(this.rootDir, '.gclient')
    this.gerritMirrorsUser =
      process.env.BRAVE_USE_GERRIT_MIRRORS_USER || undefined
    this.gerritMirrorGitConfig = path.join(
      this.braveCoreDir,
      '.gitconfig_gerrit_mirror_redirect',
    )
    this.gypTargetArch = 'x64'
    this.channel = 'development'
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
    this.extraNinjaOpts = []
    this.sisoJobsLimit = undefined
    this.braveAndroidPkcs11Provider = ''
    this.braveAndroidPkcs11Alias = ''
    this.use_libfuzzer = false
    this.androidAabToApk = false

    if (this.cacheDir && !fs.existsSync(this.cacheDir)) {
      fs.mkdirSync(this.cacheDir, { recursive: true })
    }
  }

  // The value of the property `key` that can be set: what was last set, else
  // what `read` returns, read once.
  #value<K extends keyof Config>(key: K, read: () => Config[K]): Config[K] {
    if (!this.#values.has(key)) {
      this.#values.set(key, read())
    }
    return this.#values.get(key) as Config[K]
  }

  // Sets the property `key`, which only the builder sets with `--builder`.
  #set<K extends keyof Config>(key: K, value: Config[K]): void {
    assert(
      !this.#builder,
      `${key} is set, but with --builder only the builder sets it`,
    )
    this.#values.set(key, value)
  }

  // Whether the config is for a `--builder`.
  isBuilderSet(): boolean {
    return this.#builder !== undefined
  }

  get defaultBuildConfig(): string {
    return (
      this.#builder?.spec.chromium_config.build_config
      ?? this.#envConfig.getString(['default_build_config'], 'Component')
    )
  }

  get buildConfig(): string {
    return this.#value('buildConfig', () => this.defaultBuildConfig)
  }

  set buildConfig(value: string) {
    this.#set('buildConfig', value)
  }

  get chromeVersion(): string | undefined {
    return this.getProjectVersion('chrome')
  }

  get chromiumRepo(): string {
    return this.#value('chromiumRepo', () =>
      this.#envConfig.requireString([
        'projects',
        'chrome',
        'repository',
        'url',
      ]),
    )
  }

  set chromiumRepo(value: string) {
    this.#set('chromiumRepo', value)
  }

  get depotToolsDir(): string {
    return this.#envConfig.requirePath(['projects', 'depot_tools', 'dir'])
  }

  get depotToolsRepo(): string {
    return this.#envConfig.requireString([
      'projects',
      'depot_tools',
      'repository',
      'url',
    ])
  }

  get gclientVerbose(): boolean {
    return this.#value('gclientVerbose', () =>
      this.#envConfig.getBoolean(['gclient_verbose'], false),
    )
  }

  set gclientVerbose(value: boolean) {
    this.#set('gclientVerbose', value)
  }

  get disableGclientConfigUpdate(): boolean {
    return (
      !this.#builder
      && this.#envConfig.getBoolean(['disable_gclient_config_update'], false)
    )
  }

  get leanSync(): boolean {
    return this.#value('leanSync', () =>
      this.#envConfig.getBoolean(['lean_sync'], false),
    )
  }

  set leanSync(value: boolean) {
    this.#set('leanSync', value)
  }

  // `quiet` in .env: the `--quiet`/`--no-quiet` default, AI agent or not.
  get quiet(): boolean | undefined {
    return this.#value('quiet', () => this.#envConfig.getBoolean(['quiet']))
  }

  set quiet(value: boolean | undefined) {
    this.#set('quiet', value)
  }

  get gclientGlobalVars(): Record<string, any> {
    return this.#value('gclientGlobalVars', () =>
      this.#builder
        ? { ...this.#builder.spec.gclient_config.global_vars }
        : this.#envConfig.getMergedObject(['gclient', 'global_vars']),
    )
  }

  set gclientGlobalVars(value: Record<string, any>) {
    this.#set('gclientGlobalVars', value)
  }

  get targetArch(): string {
    return this.#value(
      'targetArch',
      () =>
        this.#builder?.buildTarget.targetArch
        ?? this.#envConfig.getString(['target_arch'], process.arch),
    )
  }

  set targetArch(value: string) {
    this.#set('targetArch', value)
  }

  get targetEnvironment(): string | undefined {
    return this.#value('targetEnvironment', () =>
      this.#envConfig.getString(['target_environment']),
    )
  }

  set targetEnvironment(value: string | undefined) {
    this.#set('targetEnvironment', value)
  }

  get ignorePatchVersionNumber(): boolean {
    if (this.#builder) {
      return (
        this.#builder.spec.brave_config?.ignore_patch_version_number ?? false
      )
    }
    return (
      !this.isBraveReleaseBuild()
      && this.#envConfig.getBoolean(['ignore_patch_version_number'], !isCI)
    )
  }

  get useDummyLastchange(): boolean {
    return this.#envConfig.getBoolean(['use_dummy_lastchange'], true)
  }

  get braveVersion(): string {
    return this.#getBraveVersion()
  }

  get braveIOSMarketingPatchVersion(): string | undefined {
    return this.#envConfig.getString(['brave_ios_marketing_version_patch'], '')
  }

  get releaseTag(): string | undefined {
    return this.braveVersion.split('+')[0]
  }

  get mac_signing_identifier(): string | undefined {
    return this.#value('mac_signing_identifier', () =>
      this.#envConfig.getString(['mac_signing_identifier']),
    )
  }

  set mac_signing_identifier(value: string | undefined) {
    this.#set('mac_signing_identifier', value)
  }

  get mac_installer_signing_identifier(): string {
    return this.#value('mac_installer_signing_identifier', () =>
      this.#envConfig.getString(['mac_installer_signing_identifier'], ''),
    )
  }

  set mac_installer_signing_identifier(value: string) {
    this.#set('mac_installer_signing_identifier', value)
  }

  get mac_signing_keychain(): string {
    return this.#value('mac_signing_keychain', () =>
      this.#envConfig.getString(['mac_signing_keychain'], 'login'),
    )
  }

  set mac_signing_keychain(value: string) {
    this.#set('mac_signing_keychain', value)
  }

  get notary_user(): string | undefined {
    return this.#envConfig.getString(['notary_user'])
  }

  get notary_password(): string | undefined {
    return this.#envConfig.getString(['notary_password'])
  }

  get isBraveOriginBranded(): boolean | undefined {
    return (
      !this.#builder && this.#envConfig.getBoolean(['is_brave_origin_branded'])
    )
  }

  // Parent cache directory for all internal caches (git, siso, vpython).
  get cacheDir(): string | undefined {
    return this.#envConfig.getPath(['cache_dir'])
  }

  get gitCachePath(): string | undefined {
    return this.#value(
      'gitCachePath',
      () =>
        this.#envConfig.getPath(['git_cache_path'])
        || this.resolveCacheDir('git')
        || process.env.GIT_CACHE_PATH,
    )
  }

  set gitCachePath(value: string | undefined) {
    this.#set('gitCachePath', value)
  }

  get rbeService(): string {
    return this.#value('rbeService', () =>
      this.#envConfig.getString(['rbe_service'], ''),
    )
  }

  set rbeService(value: string) {
    this.#set('rbeService', value)
  }

  get rbeTlsClientAuthCert(): string | undefined {
    return this.#envConfig.getPath(['rbe_tls_client_auth_cert'])
  }

  get rbeTlsClientAuthKey(): string | undefined {
    return this.#envConfig.getPath(['rbe_tls_client_auth_key'])
  }

  get extraGnArgs(): Record<string, any> {
    return this.#value('extraGnArgs', () =>
      this.#envConfig.getMergedObject(['gn', 'args']),
    )
  }

  set extraGnArgs(value: Record<string, any>) {
    this.#set('extraGnArgs', value)
  }

  get extraGnGenOpts(): string {
    return this.#value('extraGnGenOpts', () =>
      this.#envConfig.getString(['brave_extra_gn_gen_opts'], ''),
    )
  }

  set extraGnGenOpts(value: string) {
    this.#set('extraGnGenOpts', value)
  }

  get sisoLimits(): Record<string, any> {
    return this.#builder ? {} : this.#envConfig.getMergedObject(['siso_limits'])
  }

  get sisoCacheDir(): string | undefined {
    return this.#value(
      'sisoCacheDir',
      () =>
        this.#envConfig.getPath(['siso_cache_dir'])
        || this.resolveCacheDir('siso'),
    )
  }

  set sisoCacheDir(value: string | undefined) {
    this.#set('sisoCacheDir', value)
  }

  get braveAndroidSafeBrowsingApiKey(): string | undefined {
    return this.#envConfig.getString(['brave_safebrowsing_api_key'])
  }

  get braveAndroidDeveloperOptionsCode(): string | undefined {
    return this.#envConfig.getString(['brave_android_developer_options_code'])
  }

  get braveAndroidKeystorePath(): string | undefined {
    return this.#envConfig.getString(['brave_android_keystore_path'])
  }

  get braveAndroidKeystoreName(): string | undefined {
    return this.#envConfig.getString(['brave_android_keystore_name'])
  }

  get braveAndroidKeystorePassword(): string | undefined {
    return this.#envConfig.getString(['brave_android_keystore_password'])
  }

  get braveAndroidKeyPassword(): string | undefined {
    return this.#envConfig.getString(['brave_android_key_password'])
  }

  get useRemoteExec(): boolean {
    return this.#value('useRemoteExec', () =>
      this.#envConfig.getBoolean(['use_remoteexec'], false),
    )
  }

  set useRemoteExec(value: boolean) {
    this.#set('useRemoteExec', value)
  }

  get offline(): boolean {
    return this.#value('offline', () =>
      this.#envConfig.getBoolean(['offline'], false),
    )
  }

  set offline(value: boolean) {
    this.#set('offline', value)
  }

  get rbeReadOnly(): boolean {
    return this.#envConfig.getBoolean(['rbe_readonly'], false)
  }

  get useBraveHermeticToolchain(): boolean {
    return this.#value('useBraveHermeticToolchain', () =>
      this.#builder
        ? (this.#builder.spec.brave_config?.use_brave_hermetic_toolchain
          ?? false)
        : this.#envConfig.getBoolean(
            ['use_brave_hermetic_toolchain'],
            this.rbeService.includes('.brave.com:'),
          ),
    )
  }

  set useBraveHermeticToolchain(value: boolean) {
    this.#set('useBraveHermeticToolchain', value)
  }

  get braveIOSDeveloperOptionsCode(): string | undefined {
    return this.#envConfig.getString(['brave_ios_developer_options_code'])
  }

  get is_asan(): boolean | undefined {
    return this.#value('is_asan', () => this.#envConfig.getBoolean(['is_asan']))
  }

  set is_asan(value: boolean | undefined) {
    this.#set('is_asan', value)
  }

  get is_msan(): boolean | undefined {
    return this.#envConfig.getBoolean(['is_msan'])
  }

  get is_ubsan(): boolean | undefined {
    return this.#value('is_ubsan', () =>
      this.#envConfig.getBoolean(['is_ubsan']),
    )
  }

  set is_ubsan(value: boolean | undefined) {
    this.#set('is_ubsan', value)
  }

  get use_no_gn_gen(): boolean | undefined {
    return this.#value(
      'use_no_gn_gen',
      () => !this.#builder && this.#envConfig.getBoolean(['use_no_gn_gen']),
    )
  }

  set use_no_gn_gen(value: boolean | undefined) {
    this.#set('use_no_gn_gen', value)
  }

  // The custom deps of the chromium solution of the .gclient file. A builder
  // adds its own to package.json's.
  get chromiumCustomDeps(): Record<string, any> {
    return this.#value('chromiumCustomDeps', () => ({
      ...this.#envConfig.getMergedObject(['projects', 'chrome', 'custom_deps']),
      ...this.#builder?.spec.gclient_config.custom_deps,
    }))
  }

  set chromiumCustomDeps(value: Record<string, any>) {
    this.#set('chromiumCustomDeps', value)
  }

  // The custom vars of the chromium solution of the .gclient file. A builder
  // adds its own to package.json's, and the remote execution vars from .env.
  get chromiumCustomVars(): Record<string, any> {
    return this.#value('chromiumCustomVars', () => {
      const packageVars = this.#envConfig.getMergedObject([
        'projects',
        'chrome',
        'custom_vars',
      ])
      // For remote execution through `rbe_service`.
      const reapiVars = this.rbeService
        ? {
            'reapi_address': this.rbeService,
            'reapi_backend_config_path': 'google.star',
            'reapi_instance': 'default',
          }
        : {}
      if (this.#builder) {
        return {
          ...packageVars,
          ...this.#builder.spec.gclient_config.custom_vars,
          ...reapiVars,
        }
      }
      return {
        'checkout_pgo_profiles': this.isBraveReleaseBuild(),
        ...reapiVars,
        ...(this.is_msan ? { 'checkout_instrumented_libraries': true } : {}),
        ...packageVars,
      }
    })
  }

  set chromiumCustomVars(value: Record<string, any>) {
    this.#set('chromiumCustomVars', value)
  }

  isReleaseBuild() {
    return this.buildConfig === 'Release'
  }

  isBraveReleaseBuild() {
    if (this.#builder) {
      return this.#builder.spec.brave_config?.is_brave_release_build ?? false
    }
    const isBraveReleaseBuildValue = this.#envConfig.getNumber([
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
    return (
      !this.isAndroid()
      && (this.buildConfig === 'Debug' || this.buildConfig === 'Component')
    )
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
    const prefix = this.isBraveOriginBranded ? 'brave-origin' : 'brave-icon'
    let iconName = `${prefix}-dev-color.svg`
    if (this.isBraveReleaseBuild()) {
      if (this.channel === 'beta') {
        iconName = `${prefix}-beta-color.svg`
      } else if (this.channel === 'nightly') {
        iconName = `${prefix}-nightly-color.svg`
      } else {
        iconName = `${prefix}-release-color.svg`
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
      this.#envConfig.getString(['projects', projectName, 'revision'])
      || this.#envConfig.getString(['projects', projectName, 'tag'])
      || this.#envConfig.getString(['projects', projectName, 'branch'])
    )
  }

  getProjectRef(projectName, defaultValue = 'origin/master') {
    const revision = this.#envConfig.getString([
      'projects',
      projectName,
      'revision',
    ])
    if (revision) {
      return revision
    }

    const tag = this.#envConfig.getString(['projects', projectName, 'tag'])
    if (tag) {
      return `refs/tags/${tag}`
    }

    let branch = this.#envConfig.getString(['projects', projectName, 'branch'])
    if (branch) {
      return `origin/${branch}`
    }

    return defaultValue
  }

  resolveCacheDir(name: string): string | undefined {
    if (!this.cacheDir) {
      return undefined
    }
    return path.join(this.cacheDir, name)
  }

  updateInternal(options: UpdateOptions) {
    if (options.target_arch === 'x86') {
      this.targetArch = options.target_arch
      this.gypTargetArch = 'ia32'
    } else if (options.target_arch === 'ia32') {
      this.targetArch = 'x86'
      this.gypTargetArch = options.target_arch
    } else if (options.target_arch === 'host_cpu') {
      this.targetArch = process.arch
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
      } else if (options.target_os === 'host_os') {
        this.targetOS = this.hostOS
      } else {
        this.targetOS = options.target_os
      }
    }

    if (this.targetOS === 'android') {
      if (options.target_android_output_format) {
        this.targetAndroidOutputFormat = options.target_android_output_format
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

    // Component build is not supported on Android since cr152, fallback to
    // Static build.
    if (this.isAndroid() && this.buildConfig === 'Component') {
      this.buildConfig = 'Static'
    }

    if (options.universal) {
      this.isUniversalBinary = true
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

    if (options.lean_sync) {
      this.leanSync = options.lean_sync
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
          if (key === 'j') {
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
    const gnArgs = readArgsGn(this.srcDir, options.C, this.defaultOptions)
    Log.warn(
      '--no-gn-gen is experimental and only gn args that match command '
        + 'line options will be processed',
    )
    gnArgs.target_arch = gnArgs.target_cpu
    this.updateInternal(Object.assign({}, gnArgs, options))
    assert(!isCI)
  }

  // Points GIT_CONFIG_GLOBAL at the generated mirror git config, so every
  // process spawned from here on fetches through our Gerrit mirrors. Only
  // called by sync after generating the file, so the generator never runs with
  // GIT_CONFIG_GLOBAL pointing at its own output.
  applyGerritMirrorsGitConfig() {
    process.env.GIT_CONFIG_GLOBAL = this.gerritMirrorGitConfig
  }

  update(options: UpdateOptions) {
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
      obj[v] = this.#envConfig.getAny([v])
    }
  }

  #getBraveVersion() {
    const braveVersion = this.#envConfig.getPackageVersion()
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
      ['build'],
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

    if (this.cacheDir) {
      env.VPYTHON_VIRTUALENV_ROOT = this.resolveCacheDir('vpython')
    }

    if (!this.useBraveHermeticToolchain) {
      env.DEPOT_TOOLS_WIN_TOOLCHAIN = '0'
    } else {
      // Use hermetic toolchain only internally.
      env.USE_BRAVE_HERMETIC_TOOLCHAIN = '1'
      env.DEPOT_TOOLS_WIN_TOOLCHAIN = '1'
      env.GYP_MSVS_HASH_3bfcb536c8 = '3dce9a2ec1'
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
        this.#envConfig.getNumber(['rbe_jobs_limit'], kExecutorCount * 1.2),
        getSisoBuiltinRemoteLimit(),
      )

      // Autoninja generates -j value when RBE is enabled, adjust limits for
      // Brave-specific setup.
      Config.#capEnvNumber(env, 'NINJA_CORE_MULTIPLIER', 20)
      Config.#capEnvNumber(env, 'NINJA_CORE_LIMIT', kRemoteLimit)

      // Siso has its own limits for remote execution that do not depend on
      // NINJA_CORE_* values. Set those limits separately. See docs for more
      // details:
      // https://chromium.googlesource.com/build/+/refs/heads/main/siso/docs/environment_variables.md#siso_limits
      const defaultSisoLimits = {
        local: this.sisoJobsLimit,
        remote: this.sisoJobsLimit || kRemoteLimit,
        ...this.sisoLimits,
      }
      // Parse SISO_LIMITS from env if set (comma-separated key=value pairs).
      const envSisoLimits = new Map<string, number>()
      if (env.SISO_LIMITS) {
        for (const item of env.SISO_LIMITS.split(',')) {
          const eqIndex = item.indexOf('=')
          if (eqIndex === -1) {
            continue
          }
          const key = item.slice(0, eqIndex).trim()
          if (!key) {
            continue
          }
          const num = Number.parseInt(item.slice(eqIndex + 1).trim(), 10)
          if (Number.isFinite(num)) {
            envSisoLimits.set(key, num)
          }
        }
      }
      // Merge defaultSisoLimits with envSisoLimits ensuring that the values are
      // not greater than the default values.
      Object.entries(defaultSisoLimits).forEach(([key, defaultValue]) => {
        if (defaultValue === undefined) {
          return
        }
        const valueFromEnv = envSisoLimits.get(key) ?? defaultValue
        envSisoLimits.set(key, Math.min(defaultValue, valueFromEnv))
      })
      // Set SISO_LIMITS env var.
      env.SISO_LIMITS = Array.from(envSisoLimits.entries())
        .map(([key, value]) => `${key}=${value}`)
        .join(',')
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
    }
  }

  #outputDir: string | undefined

  get outputDir() {
    if (this.use_no_gn_gen && this.#outputDir === undefined) {
      Log.error(`You must specify output directory with -C with use_no_gn_gen`)
      process.exit(1)
    }

    const baseDir = path.join(this.srcDir, 'out')
    // Where `bots gen` writes the builder's build directory.
    if (this.#builder) {
      return path.join(baseDir, this.#builder.name)
    }
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

  get targetOS(): TargetOS {
    return this.#value('targetOS', () =>
      validTargetOS(
        this.#builder?.buildTarget.targetOS
          ?? this.#envConfig.getString(['target_os'], this.hostOS),
      ),
    )
  }

  set targetOS(value: string) {
    this.#set('targetOS', validTargetOS(value))
  }

  static #capEnvNumber(env: NodeJS.ProcessEnv, name: string, max: number) {
    const n = Number.parseInt(env[name] ?? '', 10) || max
    env[name] = Math.min(max, n).toString()
  }
}

function validTargetOS(value: string): TargetOS {
  const targetOS = validTargetOSValues.find((os) => value === os)
  if (targetOS === undefined) {
    Log.error(
      `Invalid target_os: ${value} (must be one of: ${validTargetOSValues.join(', ')})`,
    )
    process.exit(1)
  }
  return targetOS
}

function readArgsGn(srcDir, outputDir, defaultOptions: ExecOptions) {
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
    ...defaultOptions,
    skipLogging: true,
    stdio: 'pipe',
    input: script,
    encoding: 'utf8',
  })

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

let configInstance: Config | undefined

// Sets the config that `config` stands for. Only program.ts's
// installConfig(), with the checkout's config, and configMock.ts, with a
// test's, call it.
export function setConfigInstance(instance: Config): void {
  configInstance = instance
}

function getConfigInstance(): Config {
  if (!configInstance) {
    throw new Error(
      'config is read before a config is installed. Read it only from the '
        + 'actions of a command with installConfig() applied, never while a '
        + 'module loads.',
    )
  }
  return configInstance
}

// Stands for whichever config is installed, so that modules can import it
// before one is. Methods are bound to the installed config, as they read
// private fields, which cannot be reached through a proxy. Functions set on
// the config itself, such as jest spies, are returned unbound so that jest
// still recognises them.
export default new Proxy({} as Config, {
  get(_, key) {
    const target = getConfigInstance()
    const value = Reflect.get(target, key, target)
    return typeof value === 'function' && !Object.hasOwn(target, key)
      ? value.bind(target)
      : value
  },
  set: (_, key, value) =>
    Reflect.set(getConfigInstance(), key, value, getConfigInstance()),
  has: (_, key) => Reflect.has(getConfigInstance(), key),
  ownKeys: () => Reflect.ownKeys(getConfigInstance()),
  getOwnPropertyDescriptor: (_, key) =>
    Reflect.getOwnPropertyDescriptor(getConfigInstance(), key),
  defineProperty: (_, key, descriptor) =>
    Reflect.defineProperty(getConfigInstance(), key, descriptor),
  deleteProperty: (_, key) => Reflect.deleteProperty(getConfigInstance(), key),
  getPrototypeOf: () => Reflect.getPrototypeOf(getConfigInstance()),
})
