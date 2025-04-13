// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const fs = require('fs')
const path = require('path')
const config = require('../lib/config')
const updatePatches = require('../lib/updatePatches')

function loadChromiumPathFilter(filePath) {
  const configLines =
      fs.readFileSync(filePath, 'utf-8')
          .split('\n')
          .map(line => line.split('#')[0].trim())  // Removing comments.
          .filter(line => line.length > 0);

  const prefixes = [];
  const suffixes = [];
  const exactMatches = new Set();

  // This should be revisited in the future once `path.matchesGlob` is stable
  // and available in node to use, as this current implementation is a bit
  // naive.
  for (const line of configLines) {
    if (line.startsWith('*')) {
      suffixes.push(line.slice(1));
    } else if (line.endsWith('*')) {
      prefixes.push(line.slice(0, -1));
    } else {
      exactMatches.add(line);
    }
  }

  return (s) => {
    if (s.length === 0)
      return false;
    if (exactMatches.has(s))
      return false;
    if (prefixes.some(prefix => s.startsWith(prefix)))
      return false;
    if (suffixes.some(suffix => s.endsWith(suffix)))
      return false;
    return true;
  };
}

chromiumPathFilter = loadChromiumPathFilter(
    path.join(config.braveCoreDir, 'build', 'update_patches_exclusions.cfg'))

module.exports = function RunCommand (filePaths, options) {
  config.update(options)

  const chromiumDir = config.srcDir
  const v8Dir = path.join(config.srcDir, 'v8')
  const catapultDir = path.join(config.srcDir, 'third_party', 'catapult')
  const devtoolsFrontendDir = path.join(config.srcDir, 'third_party', 'devtools-frontend', 'src')
  const tfliteDir = path.join(config.srcDir, 'third_party', 'tflite', 'src')
  const searchEngineDataDir = path.join(
      config.srcDir, 'third_party', 'search_engines_data', 'resources')
  const patchDir = path.join(config.braveCoreDir, 'patches')
  const v8PatchDir = path.join(patchDir, 'v8')
  const catapultPatchDir = path.join(patchDir, 'third_party', 'catapult')
  const devtoolsFrontendPatchDir = path.join(patchDir, 'third_party', 'devtools-frontend', 'src')
  const tflitePatchDir = path.join(patchDir, 'third_party', 'tflite', 'src')
  const searchEngineDataPatchDir =
      path.join(patchDir, 'third_party', 'search_engines_data', 'resources')

  Promise.all([
    // chromium
    updatePatches(chromiumDir, patchDir, filePaths, chromiumPathFilter),
    // v8
    updatePatches(v8Dir, v8PatchDir, filePaths),
    // third_party/catapult
    updatePatches(catapultDir, catapultPatchDir, filePaths),
    // third_party/devtools-frontend/src
    updatePatches(devtoolsFrontendDir, devtoolsFrontendPatchDir, filePaths),
    // third_party/tflite/src
    updatePatches(tfliteDir, tflitePatchDir, filePaths),
    // third_party/search_engines_data
    updatePatches(searchEngineDataDir, searchEngineDataPatchDir, filePaths),
  ])
  .then(() => {
    console.log('Done.')
  })
  .catch(err => {
    console.error('Error updating patch files:')
    console.error(err)
  })
}
