// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fsSync from 'node:fs'
import fs from 'node:fs/promises'
import path from 'node:path'
import type { TranspileWebUiCliOptions } from './transpile-web-ui-command.ts'

function getIncludesString(
  options: TranspileWebUiCliOptions,
  fileList: string[],
  resourceIdPrefix: string,
  indentLevel: number,
): string {
  const indent = ' '.repeat(indentLevel * 2)
  return fileList
    .map((filePath) => {
      const rootGenDirRelativePath = path
        .relative(options.root_gen_dir, filePath)
        .replace(/\\/g, '/')
      const outputDirRelativePath = path
        .relative(options.output_dir, filePath)
        .replace(/\\/g, '/')
      const resourceId =
        resourceIdPrefix
        + outputDirRelativePath.replace(/[^a-z0-9]/gi, '_').toUpperCase()
      const resourcePath = options.resource_path_prefix
        ? // Note: We want to use forwardslash regardless of platform.
          path.posix.join(options.resource_path_prefix, outputDirRelativePath)
        : outputDirRelativePath
      return `${indent}<include name="${resourceId}" file="\${root_gen_dir}/${rootGenDirRelativePath}" resource_path="${resourcePath}" use_base_dir="false" type="BINDATA" />`
    })
    .join('\n')
}

// Generates a GRD file for a list of files.
function getGrdString(
  options: TranspileWebUiCliOptions,
  fileList: string[],
  resourceIdPrefix: string,
): string {
  const name = options.resource_name
  return `<?xml version="1.0" encoding="UTF-8"?>
<grit latest_public_release="0" current_release="1">
  <outputs>
    <output filename="grit/${name}_generated.h" type="rc_header">
      <emit emit_type='prepend'></emit>
    </output>
    <output filename="grit/${name}_generated_map.cc" type="resource_file_map_source" />
    <output filename="grit/${name}_generated_map.h" type="resource_map_header" />
    <output filename="${name}_generated.pak" type="data_package" />
  </outputs>
  <release seq="1">
    <includes>
${getIncludesString(options, fileList, resourceIdPrefix, 3)}
    </includes>
  </release>
</grit>
`
}

// Generates a GRDP file for a list of files.
function getGrdpString(
  options: TranspileWebUiCliOptions,
  fileList: string[],
  resourceIdPrefix: string,
): string {
  return `<?xml version="1.0" encoding="UTF-8"?>
<grit-part>
${getIncludesString(options, fileList, resourceIdPrefix, 1)}
</grit-part>
`
}

export async function generateWebpackGrd(options: TranspileWebUiCliOptions) {
  // remove previously generated file
  if (fsSync.existsSync(options.grd_path)) {
    await fs.unlink(options.grd_path)
  }

  // build file list from the output directory
  const filePaths: string[] = []
  for await (const fileEntry of fs.glob('**/*', {
    cwd: options.output_dir,
    withFileTypes: true,
  })) {
    if (fileEntry.isFile()) {
      filePaths.push(
        path.relative(
          process.cwd(),
          path.join(fileEntry.parentPath, fileEntry.name),
        ),
      )
    }
  }

  const resourceIdPrefix = `IDR_${options.resource_name.toUpperCase()}_`
  const contents = options.grd_path.endsWith('.grdp')
    ? getGrdpString(options, filePaths, resourceIdPrefix)
    : getGrdString(options, filePaths, resourceIdPrefix)
  await fs.writeFile(options.grd_path, contents, { encoding: 'utf8' })
}
