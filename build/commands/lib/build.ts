// Copyright (c) 2017 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import config from './config.ts'
import util from './util.ts'
import branding from './branding.js'
import { runBotsGen } from './builder.ts'
import type { Builder } from './builderTypes.ts'
import type * as buildOptions from './buildOptions.ts'
import * as buildUtils from './buildUtils.ts'

export async function build(
  buildConfig = config.defaultBuildConfig,
  options: buildOptions.BuildDirOptions
    & buildOptions.TargetConfigOptions
    & buildOptions.GnArgsOptions
    & buildOptions.GnGenOptions
    & buildOptions.NinjaOptions & { builder?: Builder | undefined },
) {
  // A builder's config already builds its own build config.
  if (!options.builder) {
    config.buildConfig = buildConfig
  }
  config.update(options)
  buildUtils.checkVersionsMatch()

  util.touchOverriddenFiles()
  branding.update()
  buildUtils.ensureVsFilesMount()

  if (options.prepare_only) {
    return
  }

  // TODO(http://brave.dev/b/56449): Experimental implementation for integration
  // with our bots generator.
  if (options.builder) {
    await runBotsGen(options.builder)
    await util.buildTargets(options.builder.compileTargets)
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
