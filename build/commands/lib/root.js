const path = require('path')

let dirName = __dirname
if (process.platform === 'win32' &&
  dirName.length >= 2 &&
  dirName[1] == ':') {
  dirName = dirName[0].toUpperCase() + dirName.substr(1)
}

module.exports = path.resolve(dirName, '..', '..', '..', '..', '..')
