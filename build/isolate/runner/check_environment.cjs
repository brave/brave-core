// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Checks that Node.js can run run.ts. Plain CommonJS, so that older Node.js
// versions still report the problem.

const MIN_NODE_MAJOR_VERSION = 26

const major = Number(process.versions.node.split('.')[0])
if (major < MIN_NODE_MAJOR_VERSION) {
  console.error(
    `error: Node.js ${MIN_NODE_MAJOR_VERSION} or newer is required, found `
      + process.version,
  )
  process.exit(1)
}
