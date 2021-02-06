const config = require('../lib/config')
const util = require('../lib/util')

const gnCheck = (buildConfig = config.defaultBuildConfig, options) => {
  config.buildConfig = buildConfig
  config.update(options)
  util.run('gn', ['check', config.outputDir, '//brave/*'], config.defaultOptions)
  util.run('buildtools/checkdeps/checkdeps.py', ['brave', '--extra-repos=brave'], config.defaultOptions)
}

module.exports = gnCheck
