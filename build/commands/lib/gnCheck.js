const config = require('../lib/config')
const util = require('../lib/util')

const gnCheck = (buildConfig = config.defaultBuildConfig, options = {}) => {
  config.buildConfig = buildConfig
  config.update(options)
  util.run('gn', ['check', config.outputDir, '//brave/*'],
    config.defaultOptions)
  // TODO(bridiver) fix android deps
  if (config.targetOS !== 'ios') {
    // switch to just `//chrome` after sorting out chrome common sources
    util.run('gn', ['check', config.outputDir, '//chrome/browser/*'],
      config.defaultOptions)
    util.run('gn', ['check', config.outputDir, '//chrome/renderer/*'],
      config.defaultOptions)
    util.run('gn', ['check', config.outputDir, '//ui/webui/*'],
      config.defaultOptions)
    util.run('gn', ['check', config.outputDir,
      '//third_party/blink/renderer/*'], config.defaultOptions)
  }
  util.run('gn', ['check', config.outputDir, '//components/*'],
    config.defaultOptions)
  util.run('gn', ['check', config.outputDir, '//net/*'], config.defaultOptions)
  util.run('python3', ['buildtools/checkdeps/checkdeps.py', 'brave',
    '--extra-repos=brave', '--no-resolve-dotdot'], config.defaultOptions)
}

module.exports = gnCheck
