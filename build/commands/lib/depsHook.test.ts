// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * @jest-environment node
 */

import { spawnSync, type SpawnSyncReturns } from 'node:child_process'
import fs from 'node:fs'
import path from 'node:path'
import rootDir from './rootDir.cjs'

const srcRoot = path.join(rootDir, 'src')
const libDir = path.join(srcRoot, 'brave', 'build', 'commands', 'lib')
const depsHookPath = path.join(libDir, 'depsHook.ts')
const bundledNodeDir = path.join(srcRoot, 'third_party/node')
const fixturesDir = path.join(libDir, 'testData', 'depsHookFixtures')

// Returns the bundled //third_party/node binary path for the current host.
function getBundledNodePath(): string {
  if (process.platform === 'win32') {
    return path.join(bundledNodeDir, 'win/node.exe')
  }
  if (process.platform === 'darwin') {
    if (process.arch === 'arm64') {
      return path.join(bundledNodeDir, 'mac_arm64/node-darwin-arm64/bin/node')
    }
    return path.join(bundledNodeDir, 'mac/node-darwin-x64/bin/node')
  }
  return path.join(bundledNodeDir, 'linux/node-linux-x64/bin/node')
}

// Skip when //third_party/node is not checked out; binary path is verified
// separately once the directory exists.
function isBundledNodeAvailable(): boolean {
  return fs.existsSync(bundledNodeDir)
}

// Returns the `--import` file URL for depsHook.ts, mirroring node_action.gni.
function getDepsHookImportUrl(): string {
  const absolutePath = path.resolve(depsHookPath)
  if (process.platform === 'win32') {
    return `file:///${absolutePath.replace(/\\/g, '/')}`
  }
  return `file://${absolutePath}`
}

interface RunWithDepsHookOptions {
  rootBuildDir: string
  depFile: string
  stampFile: string
  script: string
  appendDeps?: boolean
  scriptArgs?: string[]
  omitEnvVar?:
    | 'DEPS_HOOK_ROOT_BUILD_DIR'
    | 'DEPS_HOOK_DEP_FILE'
    | 'DEPS_HOOK_STAMP_FILE'
}

// Runs bundled node with depsHook preloaded, mirroring node_action.gni.
function runWithDepsHook(
  options: RunWithDepsHookOptions,
): SpawnSyncReturns<Buffer> {
  const {
    rootBuildDir,
    depFile,
    stampFile,
    script,
    appendDeps = false,
    scriptArgs = [],
    omitEnvVar,
  } = options

  const env: NodeJS.ProcessEnv = {
    ...process.env,
    DEPS_HOOK_ROOT_BUILD_DIR: rootBuildDir,
    DEPS_HOOK_DEP_FILE: depFile,
    DEPS_HOOK_STAMP_FILE: stampFile,
  }
  if (appendDeps) {
    env.DEPS_HOOK_APPEND_DEPS = '1'
  }
  if (omitEnvVar !== undefined) {
    delete env[omitEnvVar]
  }

  const nodeBin = getBundledNodePath()
  const args = [`--import=${getDepsHookImportUrl()}`, script, ...scriptArgs]

  return spawnSync(nodeBin, args, {
    cwd: rootBuildDir,
    env,
    encoding: 'buffer',
  })
}

function createOutputPaths(rootBuildDir: string): {
  tempDir: string
  depFile: string
  stampFile: string
  relativeDepFile: string
  relativeStampFile: string
} {
  // Keep outputs under rootBuildDir so rebased paths stay relative on Windows
  // (path.relative returns an absolute path across drive letters).
  const tempDir = fs.mkdtempSync(path.join(rootBuildDir, '_output-'))
  const depFile = path.join(tempDir, 'test.d')
  const stampFile = path.join(tempDir, 'test.stamp')
  const relativeDepFile = path.relative(rootBuildDir, depFile)
  const relativeStampFile = path.relative(rootBuildDir, stampFile)
  if (path.isAbsolute(relativeDepFile) || path.isAbsolute(relativeStampFile)) {
    throw new Error('Output paths must be relative to the root build directory')
  }
  return {
    tempDir,
    depFile,
    stampFile,
    relativeDepFile,
    relativeStampFile,
  }
}

function escapeDepfilePath(filePath: string): string {
  const normalizedPath = filePath.replaceAll(path.sep, path.posix.sep)
  return normalizedPath.replace(/ /g, '\\ ')
}

const describeIfNode = isBundledNodeAvailable() ? describe : describe.skip

describeIfNode('depsHook integration', () => {
  let outputPaths: ReturnType<typeof createOutputPaths>

  beforeEach(() => {
    outputPaths = createOutputPaths(fixturesDir)
  })

  afterEach(() => {
    fs.rmSync(outputPaths.tempDir, { recursive: true, force: true })
  })

  it('uses the bundled node binary path from node_action.gni', () => {
    expect(fs.existsSync(getBundledNodePath())).toBe(true)
  })

  function runFixture(script: string, options: { appendDeps?: boolean } = {}) {
    return runWithDepsHook({
      rootBuildDir: fixturesDir,
      depFile: outputPaths.relativeDepFile,
      stampFile: outputPaths.relativeStampFile,
      script,
      ...options,
    })
  }

  it('tracks loaded modules and writes an empty stamp file', () => {
    const result = runFixture('basic/entry.js')

    expect(result.status).toBe(0)
    expect(fs.existsSync(outputPaths.stampFile)).toBe(true)
    expect(fs.readFileSync(outputPaths.stampFile, 'utf-8')).toBe('')

    const depfile = fs.readFileSync(outputPaths.depFile, 'utf-8')
    expect(depfile).toContain('basic/entry.js')
    expect(depfile).toContain('basic/helper.js')
    expect(depfile).toContain(escapeDepfilePath(outputPaths.relativeStampFile))
  })

  it('writes depfile in make-style format without leaving a temp file', () => {
    const result = runFixture('basic/entry.js')

    expect(result.status).toBe(0)

    const depfile = fs.readFileSync(outputPaths.depFile, 'utf-8')
    const stampTarget = escapeDepfilePath(outputPaths.relativeStampFile)
    const lines = depfile.replace(/\r\n/g, '\n').trimEnd().split('\n')
    expect(lines).toEqual([
      `${stampTarget}: \\`,
      ' basic/entry.js \\',
      ' basic/helper.js',
    ])
    expect(fs.existsSync(`${outputPaths.depFile}.tmp`)).toBe(false)
  })

  it('uses POSIX path separators in dependency paths', () => {
    const result = runFixture('basic/entry.js')

    expect(result.status).toBe(0)

    const depfile = fs.readFileSync(outputPaths.depFile, 'utf-8')
    expect(depfile).toContain('basic/entry.js')
    expect(depfile).toContain('basic/helper.js')
    expect(depfile).not.toContain('basic\\entry.js')
    expect(depfile).not.toContain('basic\\helper.js')
  })

  it('removes a pre-existing depfile on hook load', () => {
    fs.mkdirSync(path.dirname(outputPaths.depFile), { recursive: true })
    fs.writeFileSync(outputPaths.depFile, 'stale: dep\n', 'utf-8')

    const result = runFixture('noop.js')

    expect(result.status).toBe(0)

    const depfile = fs.readFileSync(outputPaths.depFile, 'utf-8')
    expect(depfile).not.toContain('stale')
    expect(depfile).toContain('noop.js')
  })

  it('appends tracked deps to an existing depfile when DEPS_HOOK_APPEND_DEPS=1', () => {
    const result = runFixture('writesDepfile.js', { appendDeps: true })

    expect(result.status).toBe(0)

    const depfile = fs.readFileSync(outputPaths.depFile, 'utf-8')
    expect(depfile.startsWith('existing: dep\n\n')).toBe(true)
    expect(depfile).toContain('writesDepfile.js')
    expect(depfile).toContain(escapeDepfilePath(outputPaths.relativeStampFile))
  })

  it('does nothing in append mode when the depfile does not exist', () => {
    const result = runFixture('noop.js', { appendDeps: true })

    expect(result.status).toBe(0)
    expect(fs.existsSync(outputPaths.depFile)).toBe(false)
    expect(fs.existsSync(outputPaths.stampFile)).toBe(false)
  })

  it('escapes spaces in dependency paths', () => {
    const result = runFixture('space dir/module.js')

    expect(result.status).toBe(0)

    const depfile = fs.readFileSync(outputPaths.depFile, 'utf-8')
    expect(depfile).toContain(escapeDepfilePath('space dir/module.js'))
    expect(depfile).not.toMatch(/[^\\] space dir/)
  })

  it('fails when a required env var is missing', () => {
    const result = runWithDepsHook({
      rootBuildDir: fixturesDir,
      depFile: outputPaths.relativeDepFile,
      stampFile: outputPaths.relativeStampFile,
      script: 'noop.js',
      omitEnvVar: 'DEPS_HOOK_ROOT_BUILD_DIR',
    })

    expect(result.status).not.toBe(0)
    expect(fs.existsSync(outputPaths.depFile)).toBe(false)
    expect(fs.existsSync(outputPaths.stampFile)).toBe(false)
  })

  it('fails when dep file path equals stamp file path', () => {
    const result = runWithDepsHook({
      rootBuildDir: fixturesDir,
      depFile: outputPaths.relativeDepFile,
      stampFile: outputPaths.relativeDepFile,
      script: 'noop.js',
    })

    expect(result.status).not.toBe(0)
    expect(fs.existsSync(outputPaths.stampFile)).toBe(false)
  })
})
