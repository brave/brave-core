const config = require('../lib/config')
const util = require('../lib/util')

const applyPatches = (buildConfig = config.defaultBuildConfig, options = {}) => {
  async function RunCommand () {
    config.buildConfig = buildConfig
    config.update(options)
    await util.applyPatches(options.printPatchFailuresInJson)
  }

  RunCommand().catch((err) => {
    console.error(err)
    process.exit(1)
  })
}

module.exports = applyPatches
