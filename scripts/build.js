/* global exec */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const tasks = require('./tasks')

tasks.replaceWebpack()
console.log('[Copy assets]')
console.log('-'.repeat(80))
tasks.copyAssets('build')

console.log('[Webpack Build]')
console.log('-'.repeat(80))
if (exec('webpack --config webpack/prod.config.js --progress --profile --colors').code !== 0) {
  echo('Erorr: webpack failed')
  exit(1)
}
