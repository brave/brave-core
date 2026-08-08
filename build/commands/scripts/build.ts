// Copyright (c) 2017 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { Command } from 'commander'
import * as build from '../lib/build.ts'

build
  .addBuildOptions(
    build.addGnGenOptions(
      build.addBuildConfigOptions(
        build.addGNArgsOptions(
          new Command().description('Build Brave browser'),
        ),
      ),
    ),
  )
  .action(build.build)
  .parseAsync()
