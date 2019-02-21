/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const fs = require('fs')
const ChromeExtension = require('crx')
/* eslint import/no-unresolved: 0 */
const name = require('../build/manifest.json').name
const argv = require('minimist')(process.argv.slice(2))

const keyPath = argv.key || 'key.pem'
const existsKey = fs.existsSync(keyPath)
const crx = new ChromeExtension({
  appId: argv['app-id'],
  codebase: argv.codebase,
  privateKey: existsKey ? fs.readFileSync(keyPath) : null
})

crx.load('build')
  .then(() => crx.loadContents())
  .then((archiveBuffer) => {
    fs.writeFile(`${name}.zip`, archiveBuffer)

    if (!argv.codebase || !existsKey) return
    crx.pack(archiveBuffer).then((crxBuffer) => {
      const updateXML = crx.generateUpdateXML()

      fs.writeFile('update.xml', updateXML)
      fs.writeFile(`${name}.crx`, crxBuffer)
    })
  })
