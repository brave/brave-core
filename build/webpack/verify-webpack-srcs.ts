// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs/promises'
import path from 'node:path'
import type { TranspileWebUiCliOptions } from './transpile-web-ui-command.ts'

// Verifies that all files webpack imports or their containing folders are
// listed in the imports_from section defined in transpile_web_ui targets.
// Doing it here allows us to not only verify the completeness but also to
// generate an actionable error message.
// The list is important as it allows us to ensure gn analyze always works
// correctly. imports_from is passed as data to the underlying gn action and
// is recognised by gn analyze. This is a compromise solution to explicitly
// listing out or generating a list of files via an exec_script
// https://gn.googlesource.com/gn/+/main/docs/reference.md#cmd_analyze
export async function verifyWebpackSrcs(options: TranspileWebUiCliOptions) {
  const srcFolder = path.resolve(options.root_src_dir).replace(/\\/g, '/')

  const makeSourceAbsolute = (filePath: string): string =>
    path.resolve(filePath).replace(/\\/g, '/').replace(srcFolder, '/')

  const outDir = makeSourceAbsolute(
    path.resolve(path.dirname(path.dirname(options.root_gen_dir))),
  )

  const importPathsContent = await fs.readFile(
    options.import_paths_file,
    'utf8',
  )
  const srcRoots = JSON.parse(importPathsContent)
  if (!Array.isArray(srcRoots)) {
    throw new Error('import_paths_file must contain a JSON array')
  }

  const depfileContent = await fs.readFile(options.depfile_path, 'utf8')
  const files = depfileContent.split(/\s+/).slice(1).map(makeSourceAbsolute)

  const allRoots = [
    ...srcRoots,
    '//brave/node_modules', // handled via package.json
    outDir, // generated assets are deps and handled by gn already
    ...options.extra_modules.map(makeSourceAbsolute),
  ]

  const notContained = getNotContained(allRoots, files)

  if (notContained.length > 0) {
    console.error('error occured cross-referencing import_paths folders.')
    console.error('transpile_web_ui accessed the following files:')
    for (const file of notContained) {
      console.error('  ' + file)
    }

    if (srcRoots.length > 0) {
      console.error(
        'However they are not listed as imports_from in target. imports_from conatains:',
      )
      for (const root of srcRoots) {
        console.error('  ' + root)
      }
    } else {
      console.error('However imports_from is empty')
    }

    console.error(
      'fix this issue by adding the containing source folders into the transpile_web_ui target imports_from section',
    )
    process.exit(1)
  }
}

function getNotContained(roots: string[], testPaths: string[]): string[] {
  // Check whether all given paths are contained within the source roots.
  // Returns list of paths that were not contained.
  return testPaths.filter(
    (testPath) => !roots.some((root) => testPath.startsWith(root)),
  )
}
