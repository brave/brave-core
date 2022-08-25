const path = require('path')
const config = require('../lib/config')
const util = require('../lib/util')
const { extensionScriptPaths } = require('./l10nUtil')
const l10nUtil = require('./l10nUtil')

const pullL10n = (options) => {
  const cmdOptions = config.defaultOptions
  cmdOptions.cwd = config.braveCoreDir
  if (options.extension) {
    const getMessages = l10nUtil.extensionScriptPaths[options.extension]
    if (!getMessages) {
      console.error('Unknown extension: ', options.extension, 'Valid extensions are: ', Object.keys(extensionScriptPaths).join(", "))
      process.exit(1)
      return
    }

    getMessages(options.extension_path).forEach((sourceStringPath) => {
      util.run('python', ['script/pull-l10n.py', '--source_string_path', sourceStringPath], cmdOptions)
    })
    return
  }

  // Revert to originals before string replacement because original grd(p)s are
  // overwritten with modified versions from ./src/brave during build.
  const srcDir = config.srcDir
  const targetFilesForReset = ["*.grd", "*.grdp", "*.xtb"]
  targetFilesForReset.forEach((targetFile) => {
    util.run('git', ['checkout', '--', targetFile], { cwd: srcDir })
  })

  l10nUtil.getBraveTopLevelPaths().forEach((sourceStringPath) => {
    if (!options.grd_path || sourceStringPath.endsWith(path.sep + options.grd_path)) {
      let cmd_args = ['script/pull-l10n.py', '--source_string_path', sourceStringPath]
      if (options.debug)
        cmd_args.push('--debug')
      util.run('python3', cmd_args, cmdOptions)
    }
  })
}

module.exports = pullL10n
