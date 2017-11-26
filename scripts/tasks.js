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

exports.copyAssets = (type) => {
  const env = type === 'build' ? 'prod' : type
  rm('-rf', type)
  mkdir(type)
  cp(`app/manifest.${env}.json`, `${type}/manifest.json`)
  cp('-R', 'app/_locales/', type)
  cp('-R', 'app/assets/*', type)
  exec(`pug -O "{ env: '${env}' }" -o ${type} app/views/`)
}
