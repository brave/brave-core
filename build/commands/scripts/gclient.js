// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import util from '../lib/util.ts'
import { program } from '../lib/program.ts'

// Every argument, --help included, is gclient's.
program
  .helpOption(false)
  .allowUnknownOption()
  .allowExcessArguments()
  .action(() => util.runGclient(process.argv.slice(2)))
  .parse()
