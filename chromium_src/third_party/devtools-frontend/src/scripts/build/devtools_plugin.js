// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs';
import path from 'node:path';

const chromiumSrcDir = path.join('brave', 'chromium_src') + path.sep;

// The bundle_bundle targets don't depend on the ts_library step that
// hardlinks chromium_src overrides and //brave sources in next to the
// upstream files (see run_with_restat.py), so esbuild has to find them itself.
// See //brave/docs/devtools_frontend_patching.md.
export function resolveOverride(root, importPath) {
  // A `*.patch.js` import added by a chromium_src override, which would
  // otherwise be hardlinked in as a sibling `*.patch.ts`.
  if (importPath.endsWith('.patch.js')) {
    const tsPath = importPath.slice(0, -'.patch.js'.length) + '.ts';
    const override = path.join(root, chromiumSrcDir, path.relative(root, tsPath));
    if (fs.existsSync(override)) {
      return override;
    }
  }

  // A chromium_src override importing a plain //brave source (e.g.
  // NetworkItemView.ts -> RequestAdblockView.js), which would otherwise be
  // hardlinked in next to the override's upstream counterpart.
  if (importPath.includes(chromiumSrcDir)) {
    const bravePath = importPath.replace(chromiumSrcDir, 'brave' + path.sep);
    const braveTsPath = bravePath.endsWith('.js') ? bravePath.slice(0, -'.js'.length) + '.ts' : bravePath;
    for (const candidate of [bravePath, braveTsPath]) {
      if (fs.existsSync(candidate)) {
        return candidate;
      }
    }
  }

  return undefined;
}
