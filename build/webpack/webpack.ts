// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import webpack, { type Configuration } from 'webpack'

export async function runWebpack(config: Configuration) {
  await new Promise<void>((resolve, reject) => {
    webpack(config, (err, stats) => {
      if (err) {
        reject(err)
        return
      }

      if (stats?.hasErrors()) {
        reject(new Error(stats.toString('errors-only')))
        return
      }

      resolve()
    })
  })
}
