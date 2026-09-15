// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import path from 'node:path'
import config from './config.ts'
import * as Log from './log.ts'
import util from './util.js'

// Matches `'<entry>': '<url>',` and `'<entry>': None,` lines.
const kEntryRegex =
  /^\s*['"](?<entry>[^'"]+)['"]\s*:\s*(?:['"](?<url>[^'"]*)['"]|None)\s*,?\s*$/gm

function readGclientEntries(): string[] {
  const entriesFile = path.join(config.rootDir, '.gclient_entries')
  if (!fs.existsSync(entriesFile)) {
    return []
  }

  const content = fs.readFileSync(entriesFile, 'utf8')
  const entries: string[] = []
  for (const match of content.matchAll(kEntryRegex)) {
    const { entry, url } = match.groups ?? {}
    if (entry && url) {
      entries.push(entry)
    }
  }
  return entries
}

// Resolves the actual .git directory, handling the `gitdir:` indirection used
// by worktrees and submodules.
function resolveGitDir(checkoutDir: string): string | null {
  const dotGit = path.join(checkoutDir, '.git')
  let stat
  try {
    stat = fs.statSync(dotGit)
  } catch {
    return null
  }

  if (stat.isDirectory()) {
    return dotGit
  }

  const gitDir = /^gitdir:\s*(.+)$/m.exec(fs.readFileSync(dotGit, 'utf8'))?.[1]
  if (!gitDir) {
    return null
  }
  return path.resolve(checkoutDir, gitDir.trim())
}

function isLocalPath(url: string): boolean {
  return url.startsWith('/') || /^[a-zA-Z]:[\\/]/.test(url)
}

// Returns a reason string if the checkout references a local git cache that no
// longer exists on disk, otherwise null.
function getMissingGitCacheReason(checkoutDir: string): string | null {
  const gitDir = resolveGitDir(checkoutDir)
  if (!gitDir) {
    return null
  }

  const originUrl = util.runGit(
    checkoutDir,
    ['config', '--get', 'remote.origin.url'],
    true,
    { stdio: 'pipe', skipLogging: true },
  )
  if (originUrl && isLocalPath(originUrl) && !fs.existsSync(originUrl)) {
    return `origin ${originUrl} is missing`
  }

  const alternatesFile = path.join(gitDir, 'objects', 'info', 'alternates')
  if (fs.existsSync(alternatesFile)) {
    const missingAlternate = fs
      .readFileSync(alternatesFile, 'utf8')
      .split('\n')
      .map((line) => line.trim())
      .filter(Boolean)
      .map((objectsDir) => path.resolve(gitDir, objectsDir))
      .find((objectsDir) => !fs.existsSync(objectsDir))
    if (missingAlternate) {
      return `alternate ${missingAlternate} is missing`
    }
  }

  return null
}

// Removes checkouts listed in .gclient_entries that point at a git cache which
// has been deleted. Such checkouts make gclient fail with errors like
// "unable to normalize alternate object path" and "fatal: bad object <sha>",
// and the only way to recover is to re-clone them from scratch.
export function removeStaleCheckouts(): void {
  let removedCount = 0
  for (const entry of readGclientEntries()) {
    // Entries may be in the `<path>:<cipd package>` form.
    const checkoutDir = path.resolve(config.rootDir, entry.replace(/:.*$/, ''))
    if (!fs.existsSync(checkoutDir)) {
      continue
    }

    // The main Chromium and brave-core checkouts are never removed, they hold
    // local state that must not be lost.
    if (
      checkoutDir === path.resolve(config.srcDir)
      || checkoutDir === path.resolve(config.braveCoreDir)
    ) {
      continue
    }

    const reason = getMissingGitCacheReason(checkoutDir)
    if (!reason) {
      continue
    }

    Log.warn(`Removing stale checkout ${checkoutDir}: ${reason}`)
    fs.rmSync(checkoutDir, { recursive: true, force: true })
    removedCount++
  }

  if (removedCount > 0) {
    Log.status(
      `Removed ${removedCount} stale checkout(s), they will be re-cloned`,
    )
  }
}
