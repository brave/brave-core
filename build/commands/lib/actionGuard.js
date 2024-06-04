// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const assert = require('assert')
const fs = require('fs-extra')
const path = require('path')

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
// otherwise a rerun might be required.
class ActionGuard {
  // Path to the guard file.
  #guardFilePath
  // Cleanup closure to perform a cleanup (optional) before running the action.
  #cleanupClosure
  // Flag to indicate if the action is currently running.
  #isRunning

  constructor(guardFilePath, cleanupClosure) {
    this.#guardFilePath = guardFilePath
    this.#cleanupClosure = cleanupClosure
    this.#isRunning = false
  }

  // Check if the last action was interrupted.
  wasInterrupted() {
    assert(
      !this.#isRunning,
      'Cannot check if the action was interrupted while it is running.'
    )
    return fs.existsSync(this.#guardFilePath)
  }

  // Perform the requested action with a guard.
  run(actionClosure) {
    assert(
      !this.#isRunning,
      'Cannot run the action while it is already running.'
    )
    const wasInterrupted = this.wasInterrupted()
    if (wasInterrupted && this.#cleanupClosure) {
      this.#cleanupClosure()
      assert(
        this.wasInterrupted(),
        'Cleanup should not remove the guard as it may lead to partial cleanup and unhandled broken state.'
      )
    }
    try {
      this.#isRunning = true
      fs.ensureDirSync(path.dirname(this.#guardFilePath))
      fs.writeFileSync(this.#guardFilePath, getGuardCallStack())
      actionClosure(wasInterrupted)
      fs.unlinkSync(this.#guardFilePath)
    } finally {
      this.#isRunning = false
    }
  }
}

module.exports = ActionGuard
