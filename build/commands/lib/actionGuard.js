// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const fs = require('fs')

// This function is used to get the call stack of the guarded operation. It is
// stored in the guard file.
function getGuardCallStack() {
  const stack = new Error().stack.split('\n').slice(2)
  for (const i in stack) {
    if (!stack[i].includes('at ActionGuard.')) {
      return 'GUARD_CALLSTACK:\n' + stack.slice(i).join('\n')
    }
  }
  return ''
}

// This class is used to ensure that a given action is successfully completed,
// otherwise a cleanup might be performed.
class ActionGuard {
  constructor(guardFilePath, cleanupClosure) {
    this.guardFilePath = guardFilePath
    this.cleanupClosure = cleanupClosure
  }

  // Check if the last action was interrupted.
  isDirty() {
    return fs.existsSync(this.guardFilePath)
  }

  // If the last action was interrupted, perform the cleanup and then perform
  // the requested action with a guard.
  ensureClean(actionClosure) {
    if (this.isDirty()) {
      this.cleanupClosure()
    }
    this.run(actionClosure)
  }

  // Perform the requested action with a guard.
  run(actionClosure) {
    fs.writeFileSync(this.guardFilePath, getGuardCallStack())
    actionClosure()
    fs.unlinkSync(this.guardFilePath)
  }
}

module.exports = ActionGuard
