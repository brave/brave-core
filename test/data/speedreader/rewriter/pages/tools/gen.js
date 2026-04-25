/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const path = require('path')
const fs = require('fs')


const reportDir = path.join(__dirname, '../report/')

// only want to take "url" directories:
var dirs = fs.readdirSync(reportDir, {withFileTypes:true}).filter(d => d.isDirectory() && d.name.includes('.')).map(d => d.name)

console.log(dirs)

fs.writeFileSync(path.join(reportDir, 'reports.json'), JSON.stringify({reports: dirs}, null, 2))
