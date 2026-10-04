// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { spawnSync } from 'node:child_process'
import type { ChildProcess } from 'node:child_process'

/**
 * Stop a process and its descendants. On Windows, `child.kill()` often only
 * affects `cmd.exe`; `taskkill /T` tears down the full tree. On POSIX,
 * `pgrep -P` is used to walk descendants before SIGKILL (same idea as `/T`).
 */
export function killProcessTree(child: ChildProcess | null | undefined) {
  if (!child?.pid) {
    return
  }
  if (process.platform === 'win32') {
    killWindowsProcessSubtree(child.pid)
  } else {
    killPosixProcessSubtree(child.pid)
  }
}

/** Kill `pid` and every descendant (Windows). Uses `taskkill /T /F`. */
function killWindowsProcessSubtree(pid: number) {
  spawnSync('taskkill', ['/PID', String(pid), '/T', '/F'], {
    stdio: 'ignore',
    windowsHide: true,
  })
}

/** Kill `pid` and every descendant (POSIX). */
function killPosixProcessSubtree(pid: number) {
  for (const childPid of getDirectChildPidsOnPosix(pid)) {
    killPosixProcessSubtree(childPid)
  }
  try {
    process.kill(pid, 'SIGKILL')
  } catch {
    // Process may already have exited.
  }
}

function getDirectChildPidsOnPosix(ppid: number): number[] {
  const result = spawnSync('pgrep', ['-P', String(ppid)], {
    encoding: 'utf8',
    stdio: ['ignore', 'pipe', 'pipe'],
  })
  if (result.status !== 0) {
    return []
  }
  return result.stdout
    .trim()
    .split('\n')
    .filter(Boolean)
    .map((s) => parseInt(s, 10))
    .filter((n) => !Number.isNaN(n))
}
