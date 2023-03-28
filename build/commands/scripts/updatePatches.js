const path = require('path')
const config = require('../lib/config')
const updatePatches = require('../lib/updatePatches')

const chromiumPathFilter = (s) => s.length > 0 &&
  !s.startsWith('chrome/app/theme/default') &&
  !s.startsWith('chrome/app/theme/brave') &&
  !s.startsWith('chrome/app/theme/chromium') &&
  !s.endsWith('.png') && !s.endsWith('.xtb') &&
  !s.endsWith('.grd') && !s.endsWith('.grdp') &&
  !s.endsWith('.svg') &&
  !s.endsWith('new_tab_page_view.xml') &&
  !s.endsWith('channel_constants.xml') &&
  !s.includes('google_update_idl') &&
  s !== 'ui/webui/resources/css/text_defaults_md.css'

module.exports = function RunCommand (options) {
  config.update(options)

  const chromiumDir = config.srcDir
  const v8Dir = path.join(config.srcDir, 'v8')
  const catapultDir = path.join(config.srcDir, 'third_party', 'catapult')
  const devtoolsFrontendDir = path.join(config.srcDir, 'third_party', 'devtools-frontend', 'src')
  const patchDir = path.join(config.braveCoreDir, 'patches')
  const v8PatchDir = path.join(patchDir, 'v8')
  const catapultPatchDir = path.join(patchDir, 'third_party', 'catapult')
  const devtoolsFrontendPatchDir = path.join(patchDir, 'third_party', 'devtools-frontend', 'src')

  Promise.all([
    // chromium
    updatePatches(chromiumDir, patchDir, chromiumPathFilter),
    // v8
    updatePatches(v8Dir, v8PatchDir),
    // third_party/catapult
    updatePatches(catapultDir, catapultPatchDir),
    // third_party/devtools-frontend/src
    updatePatches(devtoolsFrontendDir, devtoolsFrontendPatchDir),
  ])
  .then(() => {
    console.log('Done.')
  })
  .catch(err => {
    console.error('Error updating patch files:')
    console.error(err)
  })
}
