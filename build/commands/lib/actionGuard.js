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
  for (let i = 0; i < stack.length; i++) {
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
      'Cannot check if the action was interrupted while it is running.',
    )
    return fs.existsSync(this.#guardFilePath)
  }

  markStarted() {
    assert(
      !this.#isRunning,
      'Cannot mark the action as started while it is already running.',
    )
    this.#isRunning = true
    fs.ensureDirSync(path.dirname(this.#guardFilePath))
    fs.writeFileSync(this.#guardFilePath, getGuardCallStack())
  }

  markFinished() {
    assert(
      this.#isRunning,
      'Cannot mark the action as finished while it is not running.',
    )
    fs.unlinkSync(this.#guardFilePath)
    this.#isRunning = false
  }

  // Perform the requested action with a guard.
  run(actionClosure) {
    assert(
      !this.#isRunning,
      'Cannot run the action while it is already running.',
    )
    const wasInterrupted = this.wasInterrupted()
    if (wasInterrupted && this.#cleanupClosure) {
      this.#cleanupClosure()
      assert(
        this.wasInterrupted(),
        'Cleanup should not remove the guard as it may lead to partial cleanup and unhandled broken state.',
      )
    }
    try {
      this.markStarted()
      actionClosure(wasInterrupted)
      this.markFinished()
    } finally {
      this.#isRunning = false
    }
  }
}

module.exports = ActionGuard
