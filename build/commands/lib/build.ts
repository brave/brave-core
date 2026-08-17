// Copyright (c) 2017 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import config from './config.ts'
import util from './util.js'
import branding from './branding.js'
import type * as buildOptions from './buildOptions.ts'
import * as buildUtils from './buildUtils.ts'

export async function build(
  buildConfig = config.defaultBuildConfig,
  options: buildOptions.BuildDirOptions
    & buildOptions.TargetConfigOptions
    & buildOptions.GnArgsOptions
    & buildOptions.GnGenOptions
    & buildOptions.NinjaOptions,
) {
  config.buildConfig = buildConfig
  config.update(options)
  buildUtils.checkVersionsMatch()

  util.touchOverriddenFiles()
  branding.update()
  buildUtils.ensureVsFilesMount()
  await util.buildNativeRedirectCC()

  if (options.prepare_only) {
    return
  }

  if (config.xcode_gen_target) {
    util.generateXcodeWorkspace()
  } else {
    if (!config.use_no_gn_gen) {
      await util.generateNinjaFiles()
    }
    await util.buildTargets()
  }
}
