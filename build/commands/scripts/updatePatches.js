// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const config = require('../lib/config')
const updatePatches = require('../lib/updatePatches')

const chromiumPathFilter = (s) => s.length > 0 &&
  !s.startsWith('buildtools/reclient_cfgs') &&
  !s.startsWith('chrome/app/theme/default') &&
  !s.startsWith('chrome/app/theme/brave') &&
  !s.startsWith('chrome/app/theme/chromium') &&
  !s.startsWith('third_party/win_build_output/midl/chrome/elevation_service') &&
  !s.startsWith('third_party/win_build_output/midl/google_update') &&
  !s.endsWith('.png') && !s.endsWith('.xtb') &&
  !s.endsWith('.grd') && !s.endsWith('.grdp') &&
  !s.endsWith('.svg') &&
  !s.endsWith('new_tab_page_view.xml') &&
  !s.endsWith('channel_constants.xml') &&
  s !== 'chrome/VERSION' &&
  s !== 'ui/webui/resources/css/text_defaults_md.css'

module.exports = function RunCommand (options) {
  config.update(options)

  const chromiumDir = config.srcDir
  const v8Dir = path.join(config.srcDir, 'v8')
  const catapultDir = path.join(config.srcDir, 'third_party', 'catapult')
  const devtoolsFrontendDir = path.join(config.srcDir, 'third_party', 'devtools-frontend', 'src')
  const ffmpegDir = path.join(config.srcDir, 'third_party', 'ffmpeg')
  const tfliteDir = path.join(config.srcDir, 'third_party', 'tflite', 'src')
  const patchDir = path.join(config.braveCoreDir, 'patches')
  const v8PatchDir = path.join(patchDir, 'v8')
  const catapultPatchDir = path.join(patchDir, 'third_party', 'catapult')
  const devtoolsFrontendPatchDir = path.join(patchDir, 'third_party', 'devtools-frontend', 'src')
  const ffmpegPatchDir = path.join(patchDir, 'third_party', 'ffmpeg')
  const tflitePatchDir = path.join(patchDir, 'third_party', 'tflite', 'src')

  Promise.all([
    // chromium
    updatePatches(chromiumDir, patchDir, chromiumPathFilter),
    // v8
    updatePatches(v8Dir, v8PatchDir),
    // third_party/catapult
    updatePatches(catapultDir, catapultPatchDir),
    // third_party/devtools-frontend/src
    updatePatches(devtoolsFrontendDir, devtoolsFrontendPatchDir),
    // third_party/ffmpeg
    updatePatches(ffmpegDir, ffmpegPatchDir),
    // third_party/tflite/src
    updatePatches(tfliteDir, tflitePatchDir)
  ])
  .then(() => {
    console.log('Done.')
  })
  .catch(err => {
    console.error('Error updating patch files:')
    console.error(err)
  })
}
