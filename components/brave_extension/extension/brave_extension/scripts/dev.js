/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const path = require('path')
const tasks = require('./tasks')
const createWebpackServer = require('webpack-httpolyglot-server')

process.env.TARGET_GEN_DIR = path.join(__dirname, '../dev/')
const buildDir = process.env.TARGET_GEN_DIR

tasks.replaceWebpack()
console.log('[Copy assets]')
console.log('-'.repeat(80))
tasks.copyAssets('dev', buildDir)

console.log('[Webpack Dev]')
console.log('-'.repeat(80))
createWebpackServer(require('../webpack/dev.config'), {
  host: 'localhost',
  port: 3000
})
