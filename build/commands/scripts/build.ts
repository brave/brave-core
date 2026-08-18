// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { program } from 'commander'
import * as buildOptions from '../lib/buildOptions.ts'
import { build } from '../lib/build.ts'

program
  .apply(buildOptions.supportBuildConfigArg)
  .apply(buildOptions.supportBuildDir)
  .apply(buildOptions.supportTargetConfig)
  .apply(buildOptions.supportGnArgs)
  .apply(buildOptions.supportGnGenOptions)
  .apply(buildOptions.supportNinjaOptions)
  .action(build)
  .parseAsync()
