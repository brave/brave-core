// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import path from 'node:path'

const depFilePath = process.env.DEPS_HOOK_DEP_FILE
if (depFilePath === undefined) {
  throw new Error('Missing DEPS_HOOK_DEP_FILE')
}

fs.mkdirSync(path.dirname(depFilePath), { recursive: true })
fs.writeFileSync(depFilePath, 'existing: dep\n', 'utf-8')
