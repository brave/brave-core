// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { Builder } from './builder.ts'

// The `init`/`sync` options the targets are chosen from. `builder` is the
// loaded `Builder` when `--builder` is given.
export interface SyncTargetOptions {
  builder?: Builder | undefined
  init?: boolean | undefined
  target_os?: string | undefined
  target_arch?: string | undefined
}

// The `target_os` and `target_cpu` lists `init`/`sync` write into `.gclient`.
export interface GclientTargets {
  targetOSList: string[]
  targetArchList: string[]
}

// The one platform `init`/`sync` pass to `config.update()`. An undefined value
// leaves the config's default in place.
export interface BuildTarget {
  targetOS: string | undefined
  targetArch: string | undefined
}
