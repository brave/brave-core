// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import config from './config.ts'

// For tests, which repoint the process-wide `config` singleton at temp
// directories. Returns a function that puts it back: fields are reset to their
// values from now (shallowly, so an object mutated in place stays mutated), and
// fields the test added or replaced with its own are dropped.
export function snapshotConfig(): () => void {
  const saved = Object.getOwnPropertyDescriptors(config)
  return () => {
    for (const key of Reflect.ownKeys(config)) {
      if (!(key in saved)) {
        Reflect.deleteProperty(config, key)
      }
    }
    Object.defineProperties(config, saved)
  }
}
