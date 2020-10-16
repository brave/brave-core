// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const execSync = require('child_process').execSync

// Grep for any usage of innerHTML / document.write.
// Ping @security-team before changing this.
const cmd = "git grep -i -E --not -e '^\\s*//' --and -e '(innerHTML|document.write)' ':(exclude)*test.cc' ':(exclude)test/*' ':(exclude)*.json' ':(exclude)build/*' ':(exclude)*browsertest*.cc'"

try {
  const stdout = execSync(cmd)
  if (stdout.length) {
    console.log(stdout.toString())
    // grep returned results, so the test fails.
    console.log('checkSecurity failed! Found uses of innerHTML/document.write.')
    process.exit(1)
  }
} catch (e) {
  if (!e.stderr.length && !e.stdout.length && e.status === 1) {
    // no grep results, so test passes
    console.log('checkSecurity passed.')
    process.exit(0)
  } else {
    console.log('checkSecurity failed. See above for error.')
    process.exit(e.status)
  }
}
