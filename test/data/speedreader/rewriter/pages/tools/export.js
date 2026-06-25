/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import path from 'node:path'
import fs from 'node:fs'
import { ZipArchive } from 'archiver'

const zip_file = fs.createWriteStream('speedreader_report.zip')
const zip = new ZipArchive()
zip.pipe(zip_file)


const reportDir = path.join(import.meta.dirname, '../report/')
zip.directory(reportDir, 'report', false)

const toolsDir = import.meta.dirname
zip.directory(toolsDir, 'tools', false)

const configDir = path.join(import.meta.dirname, '../')

zip.file(configDir + '/package.json', {name: 'package.json'})
zip.file(configDir + '/webpack.config.js', {name: 'webpack.config.js'})

zip.append(`Extract archive, type 'npm install' then type 'npm run report'. `, {name: 'README.md'})

zip.finalize()
