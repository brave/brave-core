/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 const execute = require('./lib/execute')

 const ACTION = process.env.ACTION

 let cmd = []

 switch (ACTION) {
   case 'lint':
     cmd.push('yarn lint')
     break
   case 'test-unit':
     cmd.push('yarn test-unit')
     break
 }

 execute(cmd, process.env, (err) => {
   if (err) {
     console.error('failed', err)
     process.exit(1)
   }
   console.log('done')
 })
