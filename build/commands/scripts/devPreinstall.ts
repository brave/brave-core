/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * Runs pre-install checks before pnpm installs dependencies:
 * - Validates the local development environment.
 * - Ensures all dependencies in package.json use pinned versions.
 * - Removes a non-pnpm node_modules directory if it exists.
 */

import fs from 'node:fs'
import path from 'node:path'

import '../lib/checkEnvironment.js'

// Valid pinned patterns: exact three-part semver (e.g., 1.2.3), GitHub with
// full commit hash, or pnpm catalog protocol (versions pinned in
// pnpm-workspace.yaml).
const validPinnedPattern =
  /^(\d+\.\d+\.\d+|github:[\w.-]+\/[\w.-]+#[0-9a-f]{40}|catalog:[\w.-]*)$/i

function checkUnpinnedPackageJsonDependencies(): void {
  const depScopes = [
    'dependencies',
    'devDependencies',
    'peerDependencies',
    'optionalDependencies',
  ] as const

  const packageJsonPath = path.join(process.cwd(), 'package.json')
  const packageJson = JSON.parse(
    fs.readFileSync(packageJsonPath, 'utf8'),
  ) as Partial<Record<(typeof depScopes)[number], Record<string, string>>>

  for (const scope of depScopes) {
    const deps = packageJson[scope]
    if (!deps) {
      continue
    }
    for (const [name, version] of Object.entries(deps)) {
      if (!validPinnedPattern.test(version)) {
        console.error(
          `Found unpinned dependency in package.json (${scope}): ${name} = ${version}`,
        )
        console.error(
          'Please pin all dependencies in package.json (exact X.Y.Z, github:owner/repo#<40-char-sha>, or catalog:).',
        )
        process.exit(1)
      }
    }
  }
}

checkUnpinnedPackageJsonDependencies()

const nodeModulesDir = path.join(process.cwd(), 'node_modules')
const pnpmDir = path.join(nodeModulesDir, '.pnpm')

function isNonPnpmNodeModules(): boolean {
  return (
    fs.existsSync(nodeModulesDir)
    && fs.statSync(nodeModulesDir).isDirectory()
    && !fs.existsSync(pnpmDir)
  )
}

if (isNonPnpmNodeModules()) {
  console.log(`Removing non-pnpm node_modules at ${nodeModulesDir}...`)
  fs.rmSync(nodeModulesDir, { recursive: true, force: true })
}
