const path = require('path')

let normalizedSrcDir = __dirname
if (process.platform === 'win32' &&
    normalizedSrcDir.length >= 2 &&
    normalizedSrcDir[1] == ':') {
  normalizedSrcDir = normalizedSrcDir[0].toUpperCase() + normalizedSrcDir.substr(1)
}

module.exports = path.resolve(normalizedSrcDir, '..', '..', '..', '..', '..')
