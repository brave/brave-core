const config = require('../lib/config')
const util = require('../lib/util')
const path = require('path')
const fs = require('fs-extra')

const createDist = (buildConfig = config.defaultBuildConfig, options) => {
  config.buildConfig = buildConfig
  config.update(options)
  util.updateBranding()
  // On Android CI does two builds sequentially: for aab and for apk.
  // Symbols are uploaded after 2nd build, but we need to preserve the symbols
  // from the 1st build, so don't clean here dist folder; in anyway symbols zips
  // are overwritten and brave.breakpad.syms dir is cleared before generating
  // the symbols.
  if (config.targetOS !== 'android') {
    fs.removeSync(path.join(config.outputDir, 'dist'))
  }
  if (config.shouldSign() && process.platform === 'win32') {
    // Sign binaries used for widevine sig file generation.
    // Other binaries will be done during the create_dist.
    // Then, both are merged when archive for installer is created.
    util.signWinBinaries()

    if (config.enableCDMHostVerification()) {
      util.generateWidevineSigFiles()
    }
  }
  config.buildTarget = 'create_dist'
  util.buildTarget()
}

module.exports = createDist
