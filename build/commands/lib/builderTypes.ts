// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// TODO(http://brave.dev/b/56449): Experimental implementation for integration
// with our bots generator.

// A builder's spec: what it builds and syncs for, as bots declares it. Its GN
// args stay private to `bots gen`.
export interface BuilderSpec {
  builder_group: string
  chromium_config: {
    target_platform: string
    host_platform?: string | undefined
    build_config?: string | undefined
    target_arch?: string | undefined
  }
  gclient_config: {
    // What the checkout syncs besides the host, which gclient always syncs.
    target_os: string[]
    target_cpu: string[]
    // What the builder adds to the chromium solution's, on top of
    // package.json's.
    custom_deps?: Record<string, string | null> | undefined
    custom_vars?: Record<string, string | boolean> | undefined
    // The variables the builder sets at the top of `.gclient`.
    global_vars?: Record<string, string | boolean | number> | undefined
  }
  // What the build tooling takes from the builder rather than `.env`; each
  // is false when the spec leaves it out.
  // TODO(http://brave.dev/b/56449): Each of these is a leak in the build
  // abstraction: a Brave setting the build tooling acts on outside GN, which
  // GN should decide from the builder's args instead.
  brave_config?:
    | {
        is_brave_release_build?: boolean | undefined
        ignore_patch_version_number?: boolean | undefined
        use_brave_hermetic_toolchain?: boolean | undefined
      }
    | undefined
}

export interface Builder {
  name: string
  spec: BuilderSpec
  // TODO(http://brave.dev/b/56449): The same abstraction leak: GN's
  // `target_os` and `target_cpu`, read from the builder's `gn-args.json`
  // until `init`/`sync` no longer need a target to build for.
  buildTarget: { targetOS: string; targetArch: string }
  // TODO(http://brave.dev/b/56449): An abstraction leak: `build/commands`
  // reading a GN arg's value is an antipattern. GN's `is_brave_origin_branded`
  // is read from `gn-args.json` only because the build tooling still copies
  // the branding files in outside GN.
  isBraveOriginBranded: boolean
  // Its `$brave_siso` properties.
  siso: { remoteJobs?: number | undefined; reapiPriority?: number | undefined }
  // What it compiles: the binaries its tests run and its additional compile
  // targets, from its targets spec.
  compileTargets: string[]
}
