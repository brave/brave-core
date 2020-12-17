const config = require('../lib/config')
const util = require('../lib/util')
const path = require('path')
const fs = require('fs-extra')

const createDist = (buildConfig = config.defaultBuildConfig, options) => {
  config.buildConfig = buildConfig
  config.update(options)
  util.updateBranding()
  fs.removeSync(path.join(config.outputDir, 'dist'))
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
