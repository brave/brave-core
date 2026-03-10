// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import type { EnvConfigReader } from '../lib/envConfigReader.ts'
import assert from 'node:assert'

type Value = string | number | boolean | { [key: string]: Value }
type ArgValue = Value | Value[] | undefined
type Args = Record<string, ArgValue>
type ConfigMethod<T> = () => ConfigDescriptor<T>

type ArgsContext<T> = {
  [P in keyof T]?: true
} & { env: EnvConfigReader; isCI: boolean }

type AllMethodsReturnConfig<T> = {
  [K in keyof T]: T[K] extends (...args: any[]) => ConfigDescriptor<T>
    ? Parameters<T[K]> extends []
      ? ConfigMethod<T>
      : never
    : T[K] extends (...args: any[]) => any
      ? never
      : T[K]
}

/** Descriptor returned by each config method. Generic over the class T. */
interface ConfigDescriptor<T> {
  /**
   * Mutual-exclusion group (e.g. 'target_os'). Two configs from the same
   * group in one config list is an error.
   */
  group?: string

  /**
   * When truthy, the config is implicitly applied if no other config from the
   * same group is set. Typically a boolean expression evaluated at load time
   * (e.g. `process.arch === 'arm64'`).
   * Requires `group` to be set.
   */
  groupDefault?: boolean

  /**
   * References to other config methods this config depends on.
   * Order matters: later entries override earlier ones for the same GN key.
   * Grouped deps are overridable (skipped when the group is already
   * satisfied); ungrouped deps are always applied.
   * See "Overridable and Hard Dependencies" below.
   */
  configs?: Array<ConfigMethod<T>>

  /**
   * GN argument key-value pairs. Plain object for static args, or a callback
   * `(ctx) => Args` for conditional args. `ctx` contains config participation
   * flags plus `ctx.env` for env-backed value lookup during arg resolution. A
   * config's own args always override args inherited from its `configs`
   * dependencies. `undefined` means "omit this GN arg entirely". See "Dynamic
   * Args" below.
   */
  args?: Args | ((ctx: ArgsContext<T>) => Args)

  /**
   * Marks this config as a checked-in preset. The resolved args are written to
   * `build/args/*.json`, and CI may validate that the selected config still
   * matches the committed file.
   */
  isPreset?: boolean
}

type Config = ConfigDescriptor<Configs>

class Configs {
  // ── Target OS ──────────────────────────────────────────────

  android(): Config {
    return {
      group: 'target_os',
      configs: [this._defaults, this._blink_platform_defaults],
      args: (ctx) => {
        const args: Args = {
          target_os: 'android',

          safe_browsing_mode: 2,

          // Fixes WebRTC IP leak with default option
          enable_mdns: true,

          // We want it to be enabled for all configurations
          disable_android_lint: false,
        }

        const channel = ctx.env.getString('channel')
        if (!ctx.release) {
          args.android_channel = 'default'
          args.chrome_public_manifest_package = 'com.brave.browser_default'
        } else if (channel === 'beta') {
          args.chrome_public_manifest_package = 'com.brave.browser_beta'
        } else if (channel === 'dev') {
          args.chrome_public_manifest_package = 'com.brave.browser_dev'
        } else if (channel === 'nightly') {
          args.android_channel = 'canary'
          args.chrome_public_manifest_package = 'com.brave.browser_nightly'
        }

        const targetAndroidOutputFormat = ctx.env.getString(
          'target_android_output_format',
          ctx.release ? 'aab' : 'apk',
        )

        args.target_android_output_format = targetAndroidOutputFormat

        args.android_override_version_name = ctx.env.getString(
          'android_override_version_name',
        )

        args.brave_android_developer_options_code = ctx.env.getString(
          'brave_android_developer_options_code',
        )

        args.brave_safebrowsing_api_key = ctx.env.getString(
          'brave_safebrowsing_api_key',
        )

        args.android_aab_to_apk = ctx.env.getBoolean('android_aab_to_apk')

        if (
          targetAndroidOutputFormat === 'apk'
          && (ctx.arm64 || ctx.x64)
          && ctx.isCI
        ) {
          // We want to have both 32 and 64 bit native libs in arm64/x64 apks for CI
          // Starting from cr136 it is defaulted to false.
          // For local build you can add --gn=enable_android_secondary_abi:true
          // to build both
          args.enable_android_secondary_abi = true
        }

        if (ctx.isCI && !ctx.release) {
          // We want Android CI to run Java static analyzer synchronously
          // for non-official (PR) builds.
          // It will be turned off for the official builds
          // (src/build/config/android/config.gni)
          args.android_static_analysis = 'on'
        }

        if (!ctx.dcheck_always_on) {
          args.enable_java_asserts = false
        }

        return args
      },
    }
  }

  ios(): Config {
    return {
      group: 'target_os',
      configs: [this._defaults],
      args: (ctx) => {
        const args: Args = {
          target_os: 'ios',

          ios_enable_code_signing: false,
          ios_enable_share_extension: false,
          ios_enable_credential_provider_extension: true,
          ios_enable_widget_kit_extension: false,

          // The app currently crashes on launch without disabling these 2 args:
          // https://github.com/brave/brave-browser/issues/49595
          // When this is fixed in Chromium we can remove these 2 arguments
          use_partition_alloc_as_malloc: false,
          enable_backup_ref_ptr_support: false,

          ios_provider_target: '//brave/ios/browser/providers:brave_providers',

          ios_locales_pack_extra_source_patterns: [
            '%root_gen_dir%/components/brave_components_strings_',
          ],
          ios_locales_pack_extra_deps: ['//brave/components/resources:strings'],

          // Configure unit tests to run outside of chromium infra
          // https://source.chromium.org/chromium/chromium/src/+/main:ios/build/bots/scripts/README.md
          enable_run_ios_unittests_with_xctest: true,
        }

        args.target_environment = ctx.env.getString('target_environment')
        args.brave_ios_marketing_version_patch = ctx.env.getString(
          'brave_ios_marketing_version_patch',
        )

        assert(!ctx.component, 'Component builds are not supported for iOS')

        if (!ctx.release) {
          // When building locally iOS needs dSYMs in order for Xcode to map source
          // files correctly since we are using a framework build
          args.enable_dsyms = true
        }

        args.brave_ios_developer_options_code = ctx.env.getString(
          'brave_ios_developer_options_code',
        )

        return args
      },
    }
  }

  linux(): Config {
    return {
      group: 'target_os',
      groupDefault: process.platform === 'linux',
      configs: [
        this._defaults,
        this._blink_platform_defaults,
        this._desktop_defaults,
      ],
      args: (ctx) => {
        const args: Args = {
          target_os: 'linux',
        }

        if (
          (ctx.release && ctx.x86)
          || (!ctx.debug && !ctx.component && !ctx.release)
        ) {
          // Adjust symbol_level in Linux builds:
          // 1. Set minimal symbol level to workaround size restrictions: on Linux x86,
          //    ELF32 cannot be > 4GiB.
          // 2. Enable symbols in Static builds. By default symbol_level is 0 in this
          //    configuration. symbol_level = 2 cannot be used because of "relocation
          //    R_X86_64_32 out of range" errors.
          args.symbol_level = 1
        }

        if (ctx.release) {
          // For Linux Release builds, upstream doesn't want to use symbol_level = 2
          // unless use_debug_fission is set. However, they don't set it when a
          // cc_wrapper is used. Since we use cc_wrapper we need to set it manually.
          args.use_debug_fission = true
        }

        if (!ctx.x86) {
          // Include vaapi support
          // TODO: Consider setting use_vaapi_x11 instead of use_vaapi. Also
          // consider enabling it for x86 builds. See
          // https://github.com/brave/brave-browser/issues/1024#issuecomment-1175397914
          args.use_vaapi = true
        }

        return args
      },
    }
  }

  mac(): Config {
    return {
      group: 'target_os',
      groupDefault: process.platform === 'darwin',
      configs: [
        this._defaults,
        this._blink_platform_defaults,
        this._desktop_defaults,
      ],
      args: (ctx) => {
        const args: Args = {
          target_os: 'mac',
          // Keychain service/account are now configurable at runtime.
          allow_runtime_configurable_key_storage: true,
        }

        // Always use hermetic xcode for macos when available.
        args.use_system_xcode = ctx.env.getBoolean('use_system_xcode')

        args.is_universal_binary = ctx.env.getBoolean('is_universal_binary')

        return args
      },
    }
  }

  win(): Config {
    return {
      group: 'target_os',
      groupDefault: process.platform === 'win32',
      configs: [
        this._defaults,
        this._blink_platform_defaults,
        this._desktop_defaults,
      ],
      args: { target_os: 'win' },
    }
  }

  // ── CPU Architecture ────────────────────────────────────────

  arm64(): Config {
    return {
      group: 'target_cpu',
      groupDefault: process.arch === 'arm64',
      args: { target_cpu: 'arm64' },
    }
  }

  arm(): Config {
    return {
      group: 'target_cpu',
      args: { target_cpu: 'arm' },
    }
  }

  x64(): Config {
    return {
      group: 'target_cpu',
      groupDefault: process.arch === 'x64',
      args: { target_cpu: 'x64' },
    }
  }

  x86(): Config {
    return {
      group: 'target_cpu',
      args: {
        target_cpu: 'x86',
      },
    }
  }

  // ── Linkage ───────────────────────────────────────────────

  static(): Config {
    return {
      group: 'linkage',
      args: {
        is_component_build: false,
      },
    }
  }

  component(): Config {
    return {
      group: 'linkage',
      args: {
        is_component_build: true,
      },
    }
  }

  // ── Build Type ────────────────────────────────────────────

  release(): Config {
    return {
      group: 'build_type',
      args: {
        is_official_build: true,
        enable_update_notifications: true,
        enable_updater: true,
      },
    }
  }

  debug(): Config {
    return {
      group: 'build_type',
      args: (ctx) => {
        const args: Args = {
          is_debug: true,
        }

        if (!ctx.component && !ctx.android && !ctx.ios) {
          args.enable_profiling = true
        }

        return args
      },
    }
  }

  // ── Brave Release ────────────────────────────────────────

  brave_release(): Config {
    return {
      group: 'brave_release',
      configs: [this.release],
    }
  }

  non_brave_release(): Config {
    return {
      group: 'brave_release',
      groupDefault: true,
      args: (_ctx) => ({
        chrome_pgo_phase: 0,

        // Don't randomize mojom message ids. When randomization is enabled, all
        // Mojo targets are rebuilt (~23000) on each version bump.
        enable_mojom_message_id_scrambling: false,
      }),
    }
  }

  // ── Build Omaha ────────────────────────────────────────────

  build_omaha(): Config {
    return {
      args: (ctx) => ({
        build_omaha: true,
        ...ctx.env.pickStrings('tag_ap', 'tag_installdataindex'),
      }),
    }
  }

  last_chrome_installer(): Config {
    return {
      args: (ctx) => ({
        last_chrome_installer: ctx.env.getString('last_chrome_installer'),
      }),
    }
  }

  // ── Sanitizers ─────────────────────────────────────────────

  dcheck_always_on(): Config {
    return {
      args: {
        dcheck_always_on: true,
      },
    }
  }

  asan(): Config {
    return {
      configs: [this.dcheck_always_on],
      args: {
        is_asan: true,
        enable_full_stack_frames_for_profiling: true,
        v8_enable_verify_heap: true,
      },
    }
  }

  lsan(): Config {
    return {
      configs: [this.asan],
      args: {
        is_lsan: true,
      },
    }
  }

  ubsan(): Config {
    return {
      configs: [this.dcheck_always_on],
      args: {
        is_ubsan: true,
        is_ubsan_vptr: true,
        is_ubsan_no_recover: true,
      },
    }
  }

  msan(): Config {
    return {
      configs: [this.dcheck_always_on],
      args: {
        is_msan: true,
      },
    }
  }

  // ── Branding ───────────────────────────────────────────────

  branding_brave(): Config {
    return {
      group: 'branding',
      groupDefault: true,
      args: {
        branding_path_component: 'brave',
        branding_path_product: 'brave',
      },
    }
  }

  branding_brave_origin(): Config {
    return {
      group: 'branding',
      args: {
        branding_path_component: 'brave_origin',
        branding_path_product: 'brave_origin',
      },
    }
  }

  // ── Signing ────────────────────────────────────────────

  sign(): Config {
    return {
      args: (ctx): Args => {
        if (ctx.mac) {
          return {
            ...ctx.env.pickStrings(
              'mac_signing_identifier',
              'mac_installer_signing_identifier',
            ),
            mac_signing_keychain: ctx.env.getString(
              'mac_signing_keychain',
              'login',
            ),
          }
        } else if (ctx.android) {
          return ctx.env.pickStrings(
            'brave_android_keystore_path',
            'brave_android_keystore_name',
            'brave_android_keystore_password',
            'brave_android_key_password',
            'brave_android_pkcs11_provider',
            'brave_android_pkcs11_alias',
          )
        } else {
          return {}
        }
      },
    }
  }

  notarize(): Config {
    return {
      configs: [this.sign],
      args: (ctx) => ({
        notarize: true,
        ...ctx.env.pickStrings('notary_user', 'notary_password'),
      }),
    }
  }

  // ── Internal Configs ───────────────────────────────────

  _defaults(): Config {
    return {
      configs: [
        this._rbe_config,
        this._clang_coverage,
        this._devtools_typecheck,
      ],
      args: (ctx) => {
        const args: Args = {
          disable_fieldtrial_testing_config: true,
          root_extra_deps: ['//brave'],

          // Default value currently causes multiple errors of
          // Input to targets not generated by a dependency
          // Chromium change: 3c46aa800cbd4e21aeb08ac7c1222ce33d5c902e
          // This has been disabled after some discussion deciding it is not relevant for
          // our use case since it was introduced in M140.
          translate_genders: false,

          // Chromium_src overrides do not work with precompiled headers well.
          // Before Siso they were always disabled as we always used redirect_cc as a
          // cc_wrapper, but with Siso we need to disable it explicitly.
          enable_precompiled_headers: false,

          enable_pseudolocales: false,

          // Our copy of signature_generator.py doesn't support --ignore_missing_cert:
          ignore_missing_widevine_signing_cert: false,
        }

        if (!ctx.brave_release) {
          args.use_dummy_lastchange = ctx.env.getBoolean(
            'use_dummy_lastchange',
            true,
          )
        }

        return args
      },
    }
  }

  _devtools_typecheck(): Config {
    return {
      args: (ctx) => {
        const args: Args = {}
        if (ctx.isCI && !ctx.release && !ctx.android && !ctx.ios) {
          // Devtools: Now we patch devtools frontend, so it is useful to see
          // if something goes wrong on CI builds.
          args.devtools_skip_typecheck = false
        }

        return args
      },
    }
  }

  _clang_coverage(): Config {
    return {
      args: (ctx) => {
        const args: Args = {
          use_clang_coverage: true,
        }

        if (ctx.release) {
          const buildDir = path.relative(
            ctx.env.getPath('src_dir'),
            ctx.env.getPath('output_dir'),
          )
          args.coverage_instrumentation_input_file = `//${buildDir}/files-to-instrument.txt`
        }

        return args
      },
    }
  }

  _blink_platform_defaults(): Config {
    return {
      args: {
        proprietary_codecs: true,
        ffmpeg_branding: 'Chrome',
        enable_widevine: true,
      },
    }
  }

  _desktop_defaults(): Config {
    return {
      args: {
        safe_browsing_mode: 1,
      },
    }
  }

  _rbe_config(): Config {
    return {
      args: (ctx) => {
        const useRemoteExec = ctx.env.get(['use_remoteexec'], false)
        const useSiso = ctx.env.get(['use_siso'], true)
        const useReclient = ctx.env.get(
          ['use_reclient'],
          useRemoteExec && !useSiso,
        )
        const args: Args = {
          use_remoteexec: useRemoteExec,
          use_siso: useSiso,
          use_reclient: useReclient,
        }
        if (!useSiso) {
          if (useRemoteExec) {
            args.reclient_bin_dir = ctx.env.getPath('reclient_bin_dir')
          } else {
            args.cc_wrapper = path.join(
              ctx.env.getPath('reclient_bin_dir'),
              'redirect_cc',
            )
          }
        }

        if (useRemoteExec && ctx.ios) {
          // RBE expects relative paths in dSYMs
          args.strip_absolute_paths_from_debug_symbols = true
        }

        if (ctx.mac && ctx.release && !ctx.brave_release) {
          // Don't create dSYMs in non-true Release builds. dSYMs should be disabled
          // in order to have relocatable compilation so RBE can share the cache
          // across multiple build directories. Enabled dSYMs enforce absolute
          // paths, which makes RBE cache unusable.
          args.enable_dsyms = false
        }

        return args
      },
    }
  }
}

export default new Configs() satisfies AllMethodsReturnConfig<Configs>
