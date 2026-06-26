// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs/promises'
import path from 'node:path'

import {
  command,
  type TranspileWebUiCliOptions,
} from './transpile-web-ui-command.ts'
import { createWebpackConfig } from './webpack.config.ts'
import { runWebpack } from './webpack.ts'
import { generateWebpackGrd } from './gen-webpack-grd.ts'
import { verifyWebpackSrcs } from './verify-webpack-srcs.ts'

async function main(options: TranspileWebUiCliOptions) {
  await cleanDirectory(options.output_dir)
  await runWebpack(createWebpackConfig(options))
  await generateWebpackGrd(options)
  await verifyWebpackSrcs(options)
}

// Clean directory recursively, but not the directory itself.
async function cleanDirectory(outputDir: string) {
  await Promise.all(
    (await fs.readdir(outputDir)).map((fileEntry) =>
      fs.rm(path.join(outputDir, fileEntry), {
        recursive: true,
        force: true,
      }),
    ),
  )
}

command.action(main).parseAsync()
