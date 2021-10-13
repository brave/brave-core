const path = require('path')

let normalizedDirName = __dirname
if (process.platform === 'win32' &&
  normalizedDirName.length >= 2 &&
  normalizedDirName[1] == ':') {
  normalizedDirName = normalizedDirName[0].toUpperCase() + normalizedDirName.substr(1)
}

module.exports = path.resolve(normalizedDirName, '..', '..', '..', '..', '..')
