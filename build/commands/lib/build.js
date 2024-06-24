const config = require('../lib/config')
const util = require('../lib/util')
const path = require('path')
const fs = require('fs-extra')
const Log = require('../lib/logging')

/**
 * Checks to make sure the src/chrome/VERSION matches brave-core's package.json version
 */
const checkVersionsMatch = () => {
  const srcChromeVersionDir = path.resolve(path.join(config.srcDir, 'chrome', 'VERSION'))
  const versionData = fs.readFileSync(srcChromeVersionDir, 'utf8')
  const re = /MAJOR=(\d+)\s+MINOR=(\d+)\s+BUILD=(\d+)\s+PATCH=(\d+)/
  const found = versionData.match(re)
  const braveVersionFromChromeFile = `${found[2]}.${found[3]}.${found[4]}`
  if (braveVersionFromChromeFile !== config.braveVersion) {
    // Only a warning. The CI environment will choose to proceed or not within its own script.
    Log.warn(
      `Version files do not match!\n` +
      `src/chrome/VERSION: ${braveVersionFromChromeFile}\n` +
      `brave-core configured version: ${config.braveVersion}\n` +
      `Did you forget to sync?`)
  }
}

const build = async (buildConfig = config.defaultBuildConfig, options = {}) => {
  config.buildConfig = buildConfig
  config.update(options)
  checkVersionsMatch()

  util.touchOverriddenFiles()
  util.updateBranding()
  await util.buildNativeRedirectCC()

  if (options.prepare_only) {
    return
  }

  if (config.xcode_gen_target) {
    util.generateXcodeWorkspace()
  } else {
    if (options.no_gn_gen === undefined)
      await util.generateNinjaFiles()
    await util.buildTargets()
  }
}

module.exports = build
