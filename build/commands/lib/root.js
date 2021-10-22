const path = require('path')
const fs = require('fs')

let dirName = __dirname

// Use fs.realpathSync to normalize the path(__dirname could be c:\.. or C:\..).
if (process.platform === 'win32') {
  dirName = fs.realpathSync.native(dirName)
}

module.exports = path.resolve(dirName, '..', '..', '..', '..', '..')
