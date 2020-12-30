const path = require('path')
const config = require('../lib/config')
const util = require('../lib/util')
const {braveTopLevelPaths, getEthereumRemoteClientPaths, getGreaselionScriptPaths} = require('./l10nUtil')

const pullL10n = (options) => {
  const cmdOptions = config.defaultOptions
  cmdOptions.cwd = config.braveCoreDir
  if (options.extension) {
    const extensionPath = options.extension_path
    if (options.extension === 'ethereum-remote-client') {
      getEthereumRemoteClientPaths(extensionPath).forEach((sourceStringPath) => {
        util.run('python', ['script/pull-l10n.py', '--source_string_path', sourceStringPath], cmdOptions)
      })
      return
    } else if (options.extension === 'greaselion') {
      getGreaselionScriptPaths(extensionPath).forEach((sourceStringPath) => {
        util.run('python', ['script/pull-l10n.py', '--source_string_path', sourceStringPath], cmdOptions)
      })
      return
    }
    console.error('Unknown extension: ', options.extension)
    process.exit(1)
  }

  // Revert to originals before string replacement because original grd(p)s are
  // overwritten with modified versions from ./src/brave during build.
  const srcDir = config.srcDir
  const targetFilesForReset = [ "*.grd", "*.grdp", "*.xtb" ]
  targetFilesForReset.forEach((targetFile) => {
    util.run('git', ['checkout', '--', targetFile], { cwd: srcDir })
  })

  braveTopLevelPaths.forEach((sourceStringPath) => {
    if (!options.grd_path || sourceStringPath.endsWith(path.sep + options.grd_path))
      util.run('python', ['script/pull-l10n.py', '--source_string_path', sourceStringPath], cmdOptions)
  })
}

module.exports = pullL10n
