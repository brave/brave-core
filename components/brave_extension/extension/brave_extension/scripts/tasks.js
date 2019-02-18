/* global cp mkdir rm exec */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

require('shelljs/global')

exports.replaceWebpack = () => {
  const replaceTasks = [{
    from: 'webpack/replace/JsonpMainTemplate.runtime.js',
    to: 'node_modules/webpack/lib/JsonpMainTemplate.runtime.js'
  }, {
    from: 'webpack/replace/process-update.js',
    to: 'node_modules/webpack-hot-middleware/process-update.js'
  }]

  replaceTasks.forEach(task => cp(task.from, task.to))
}

exports.copyAssets = (type, outputDir) => {
  rm('-rf', outputDir)
  mkdir(outputDir)
  cp(`app/manifest.${type}.json`, `${outputDir}/manifest.json`)
  cp('-R', 'app/_locales/', outputDir)
  cp('-R', 'app/assets/*', outputDir)
  exec(`pug -O "{ env: '${type}' }" -o ${outputDir} app/views/`)
}
