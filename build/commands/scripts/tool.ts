// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../lib/checkEnvironment.js'

import { program } from 'commander'
import { command as compareBuildArtifactsCommand } from './compareBuildArtifacts.ts'

program
  .name('tool')
  .description('Hosts miscellaneous developer and CI build utilities.')
  .showHelpAfterError()
  .addCommand(compareBuildArtifactsCommand)

await program.parseAsync()
