// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import assert from 'node:assert'
import path from 'node:path'
import fs from 'mz/fs.js'

/**
 * @param {string[]} fileList
 * @returns {string}
 */
function getIncludesString(fileList) {
  return fileList
    .map((filePath) => {
      const relativePath = filePath.replace(targetDir, '')
      const fileId =
        idPrefix + relativePath.replace(/[^a-z0-9]/gi, '_').toUpperCase()
      const resourcePath = resourcePathPrefix
        ? // Note: We want to use forwardslash regardless of platform.
          path.posix.join(resourcePathPrefix, relativePath)
        : relativePath
      return `<include name="${fileId}" file="${filePath}" resource_path="${resourcePath}" use_base_dir="false" type="BINDATA" />`
    })
    .join('\n')
}

/**
 * @param {string} name
 * @param {string[]} fileList
 * @returns {string}
 */
function getGrdString(name, fileList) {
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
      ${getIncludesString(fileList)}
    </includes>
  </release>
</grit>
`
}

/**
 * Generates a GRDP file for a list of files.
 * @param {string[]} fileList The list of files to include
 * @returns {string} The contents of a GRDP file containing |fileList|
 */
function getGrdpString(fileList) {
  return `<?xml version="1.0" encoding="UTF-8"?>
<grit-part>
  ${getIncludesString(fileList)}
</grit-part>
`
}

/**
 * @param {string} dirPath
 * @returns {Promise<string[]>}
 */
async function getFileListDeep(dirPath) {
  const dirItems = await fs.readdir(dirPath)
  // get Array<string | string[]> of contents
  const dirItemGroups = await Promise.all(
    dirItems.map(async (dirItemRelativePath) => {
      const itemPath = path.join(dirPath, dirItemRelativePath)
      const stats = await fs.stat(itemPath)
      if (stats.isDirectory()) {
        return await getFileListDeep(itemPath)
      }
      if (stats.isFile()) {
        return itemPath
      }
      throw new Error(`Unexpected file type: ${itemPath}`)
    }),
  )
  // flatten to single string[]
  return dirItemGroups.reduce(
    (flatList, dirItemGroup) => flatList.concat(dirItemGroup),
    [],
  )
}

async function createDynamicGDR() {
  const gdrPath = path.join(targetDir, grdName)
  // remove previously generated file
  try {
    await fs.unlink(gdrPath)
  } catch (e) {} // eslint-disable-line no-empty
  // build file list from target dir
  const filePaths = await getFileListDeep(targetDir)
  const contents = gdrPath.endsWith('.grdp')
    ? getGrdpString(filePaths)
    : getGrdString(resourceName, filePaths)
  await fs.writeFile(gdrPath, contents, { encoding: 'utf8' })
}

// collect args
assert(process.env.RESOURCE_NAME, 'RESOURCE_NAME env variable is required')
assert(process.env.ID_PREFIX, 'ID_PREFIX env variable is required')
assert(process.env.TARGET_DIR, 'TARGET_DIR env variable is required')
assert(process.env.GRD_NAME, 'GRD_NAME env variable is required')

const resourceName = process.env.RESOURCE_NAME
const idPrefix = process.env.ID_PREFIX
let targetDir = process.env.TARGET_DIR
const grdName = process.env.GRD_NAME
const resourcePathPrefix = process.env.RESOURCE_PATH_PREFIX

if (!targetDir.endsWith(path.sep)) {
  // normalize path so relative path ignores leading path.sep
  targetDir += path.sep
}

// main
createDynamicGDR().catch((err) => {
  console.error(err)
  process.exit(1)
})
