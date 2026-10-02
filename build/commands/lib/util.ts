// Copyright (c) 2016 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import {
  spawn,
  spawnSync,
  type ChildProcess,
  type SpawnOptions,
  type SpawnSyncOptions,
} from 'node:child_process'
import readline from 'node:readline'
import os from 'node:os'
import config from './config.ts'
import fs from 'fs-extra'
import { glob, writeFile } from 'node:fs/promises'
import crypto from 'node:crypto'
import * as Log from './log.ts'
import * as GitPatcherLog from './gitPatcherLog.ts'
import assert from 'node:assert'
import ActionGuard from './actionGuard.js'
import { GitPatcher } from './gitPatcher.js'
import { getPatchedRepositories, joinSourcePath } from './repositories.ts'
import { getBuildArgs } from './buildArgs.ts'
import { isCI, isTeamcity } from './ciDetect.ts'
import * as buildDiagnostics from './buildDiagnostics.ts'
import * as processUtil from './processUtil.ts'

export type RunOptions = SpawnSyncOptions & {
  // Don't exit the process when the command fails.
  continueOnFail?: boolean | undefined
  // Don't log the command (and dump it on failure instead).
  skipLogging?: boolean | undefined
}

type RunAsyncOptions = SpawnOptions & {
  continueOnFail?: boolean | undefined
  verbose?: boolean | undefined
  onSpawn?: ((prog: ChildProcess) => void) | undefined
  onStdErrLine?: ((line: string) => void) | undefined
  onStdOutLine?: ((line: string) => void) | undefined
}

class RunAsyncError extends Error {
  readonly stderr: string
  readonly stdout: string
  readonly statusCode: number | null

  constructor(
    message: string,
    stderr: string,
    stdout: string,
    statusCode: number | null,
  ) {
    super(message)
    this.stderr = stderr
    this.stdout = stdout
    this.statusCode = statusCode
  }
}

interface GitExclusions {
  add?: string[]
  remove?: string[]
}

// Do not limit the number of listeners to avoid warnings from EventEmitter.
process.setMaxListeners(0)

async function generateInstrumentationFile(instrumentationFile: string) {
  const files = await Array.fromAsync(glob(`**/*.{cc,c,h,cpp,hpp,m,mm}`))

  const paths = files.map((x) => `../../brave/${x}`)
  await fs.mkdirp(path.dirname(instrumentationFile))
  await writeFile(instrumentationFile, paths.join('\n'), 'utf-8')
}

/**
 * Prefixes the repo a patch status came from onto its path, so that statuses
 * from the several repos patches are applied to can be told apart once logged.
 *
 * A status carries no path when the files its patch applies to could not be
 * read out of it, which is what a patch too malformed to parse reports. There
 * is nothing to prefix for those, so they are left as they are.
 *
 * Exported for tests.
 *
 * @param patchStatus The statuses, prefixed in place.
 * @param prefix The path segments of the repo they came from.
 */
export function prefixPatchPaths(
  patchStatus: { path?: string }[],
  ...prefix: string[]
) {
  for (const status of patchStatus) {
    if (status.path) {
      status.path = path.join(...prefix, status.path)
    }
  }
}

async function applyPatches(printPatchFailuresInJson?: boolean) {
  Log.progressStart('apply patches')
  // Always detect if we need to apply patches, since user may have modified
  // either chromium source files, or .patch files manually
  // Which repositories are patched, and where their patches and sources live,
  // comes from `patches/.repositories.cfg`, the same file plaster reads.
  const allPatchStatus: Awaited<ReturnType<GitPatcher['applyPatches']>> = []
  for (const repo of getPatchedRepositories()) {
    const patcher = new GitPatcher(repo.patchDir, repo.path)
    const patchStatus = await patcher.applyPatches()
    // Log status for all patches. Entries are differentiated for logging by
    // naming each source the way the whole checkout sees it, rather than the
    // way the repository holding it does.
    patchStatus.forEach((s) => {
      if (s.path) {
        s.path = joinSourcePath(repo, s.path)
      }
    })
    allPatchStatus.push(...patchStatus)
  }
  if (printPatchFailuresInJson) {
    GitPatcherLog.printFailedPatchesInJsonFormat(
      allPatchStatus,
      config.braveCoreDir,
    )
  } else {
    GitPatcherLog.allPatchStatus(allPatchStatus, 'Chromium')
  }

  const hasPatchError = allPatchStatus.some((p) => p.error)
  // Exit on error in any patch
  if (hasPatchError) {
    Log.error('Exiting as not all patches were successful!')
    process.exit(1)
  }

  util.run(
    'python3',
    [
      path.join(config.braveCoreDir, 'build', 'util', 'version.py'),
      'update',
      path.join(config.srcDir, 'chrome', 'VERSION'),
      '--brave-version',
      config.braveVersion,
    ],
    config.defaultOptions,
  )
  Log.progressFinish('apply patches')
}

function isOverrideNewer(original: string, override: string) {
  return fs.statSync(override).mtimeMs - fs.statSync(original).mtimeMs > 0
}

function updateFileUTimesIfOverrideIsNewer(original: string, override: string) {
  if (isOverrideNewer(original, override)) {
    const date = new Date()
    fs.utimesSync(original, date, date)
    console.log(original + ' is touched.')
    return true
  }
  return false
}

function deleteFileIfOverrideIsNewer(original: string, override: string) {
  if (fs.existsSync(original) && isOverrideNewer(original, override)) {
    try {
      fs.unlinkSync(original)
      console.log(original + ' has been deleted.')
      return true
    } catch (err) {
      console.error('Unable to delete file: ' + original + ' error: ', err)
      process.exit(1)
    }
  }
  return false
}

function getAdditionalGenLocation() {
  if (config.targetOS === 'android') {
    if (config.targetArch === 'arm64') {
      return 'android_clang_arm'
    } else if (config.targetArch === 'x64') {
      return 'android_clang_x86'
    }
  } else if (
    (process.platform === 'darwin' || process.platform === 'linux')
    && config.targetArch === 'arm64'
  ) {
    return 'clang_x64_v8_arm64'
  }
  return ''
}

function normalizeCommand(cmd: string, args: string[]): [string, string[]] {
  if (process.platform === 'win32') {
    args = ['/c', cmd, ...args]
    cmd = 'cmd'
  }
  return [cmd, args]
}

const util = {
  generateInstrumentationFile,
  runProcess: (
    cmd: string,
    args: string[] = [],
    options: SpawnSyncOptions = {},
    skipLogging = false,
  ) => {
    if (!skipLogging) {
      Log.command(String(options.cwd ?? ''), cmd, args)
    }
    return spawnSync(...normalizeCommand(cmd, args), options)
  },

  run: (cmd: string, args: string[] = [], options: RunOptions = {}) => {
    const { continueOnFail, skipLogging, ...cmdOptions } = options
    const prog = util.runProcess(cmd, args, cmdOptions, skipLogging)
    if (prog.status !== 0) {
      if (!continueOnFail) {
        if (skipLogging) {
          console.log(cmd, args, 'exited with status', prog.status, cmdOptions)
        }

        console.log(prog.stdout && prog.stdout.toString())
        console.error(prog.stderr && prog.stderr.toString())
        process.exit(1)
      }
    }
    return prog
  },

  runGit: (
    repoPath: string,
    gitArgs: string[],
    continueOnFail = false,
    options: RunOptions = {},
  ) => {
    const prog = util.run('git', gitArgs, {
      cwd: repoPath,
      continueOnFail,
      ...options,
    })

    if (prog.status !== 0) {
      return ''
    } else {
      return prog.stdout.toString().trim()
    }
  },

  runAsync: (
    cmd: string,
    args: string[] = [],
    options: RunAsyncOptions = {},
  ): Promise<string> => {
    const {
      continueOnFail,
      verbose,
      onSpawn,
      onStdErrLine,
      onStdOutLine,
      ...cmdOptions
    } = options
    if (verbose !== false) {
      Log.command(String(cmdOptions.cwd ?? ''), cmd, args)
    }
    return new Promise((resolve, reject) => {
      const prog = spawn(...normalizeCommand(cmd, args), cmdOptions)
      if (onSpawn) {
        onSpawn(prog)
      }
      const signalsToForward = ['SIGINT', 'SIGTERM', 'SIGQUIT', 'SIGHUP']
      const signalHandler = (s: NodeJS.Signals) => {
        prog.kill(s)
      }
      signalsToForward.forEach((signal) => {
        process.addListener(signal, signalHandler)
      })
      let stderr = ''
      let stdout = ''
      if (prog.stderr) {
        if (onStdErrLine) {
          readline
            .createInterface({
              input: prog.stderr,
              terminal: false,
            })
            .on('line', onStdErrLine)
        } else {
          prog.stderr.on('data', (data) => {
            stderr += data
          })
        }
      }
      if (prog.stdout) {
        if (onStdOutLine) {
          readline
            .createInterface({
              input: prog.stdout,
              terminal: false,
            })
            .on('line', onStdOutLine)
        } else {
          prog.stdout.on('data', (data) => {
            stdout += data
          })
        }
      }
      const closeHandler = (
        statusCode: number | null,
        signal: NodeJS.Signals | null,
      ) => {
        signalsToForward.forEach((signal) => {
          process.removeListener(signal, signalHandler)
        })
        const hasFailed = !signal && statusCode !== 0
        if (verbose && (!hasFailed || continueOnFail)) {
          if (stdout) {
            console.log(stdout)
          }
          if (stderr) {
            console.error(stderr)
          }
        }
        if (hasFailed) {
          const err = new RunAsyncError(
            `Program ${cmd} exited with error code ${statusCode}.`,
            stderr,
            stdout,
            statusCode,
          )
          reject(err)
          if (!continueOnFail) {
            console.error(err.message)
            console.error(stdout)
            console.error(stderr)
            process.exit(statusCode)
          }
          return
        } else if (signal) {
          // If the process was killed by a signal, exit with the signal number.
          process.exit(128 + os.constants.signals[signal])
        }
        resolve(stdout)
      }
      prog.on('close', (statusCode, signal) => {
        if (isCI && (statusCode || signal)) {
          // When running in CI, we delay handling process termination by 1
          // second to distinguish between two scenarios:
          // 1. A build failure (where autoninja exits with code 1)
          // 2. CI killing the process tree with SIGTERM
          //
          // Without this delay, both scenarios would appear the same since
          // SIGTERM-triggered autoninja exit would be caught by Node and
          // processed as a build failure, because autoninja has enough time to
          // handle child process termination and exit with code 1.
          //
          // The delay gives Node time to terminate directly from the SIGTERM
          // before we process the child's exit code.
          setTimeout(() => {
            closeHandler(statusCode, signal)
          }, 1000)
        } else {
          closeHandler(statusCode, signal)
        }
      })
    })
  },

  runGitAsync: function (
    repoPath: string,
    gitArgs: string[],
    verbose = false,
    logError = false,
  ) {
    return util
      .runAsync('git', gitArgs, {
        cwd: repoPath,
        verbose,
        continueOnFail: true,
      })
      .catch((err) => {
        if (logError) {
          console.error(err.message)
          console.error(`Git arguments were: ${gitArgs.join(' ')}`)
          console.log(err.stdout)
          console.error(err.stderr)
        }
        return Promise.reject(err)
      })
  },

  getGitReadableLocalRef: (repoDir: string) => {
    return util.runGit(
      repoDir,
      ['log', '-n', '1', '--pretty=format:%h%d'],
      true,
    )
  },

  calculateFileChecksum: (filename: string) =>
    crypto.hash('md5', fs.readFileSync(filename), 'hex'),

  touchOverriddenFiles: () => {
    Log.progressStart('touch original files overridden by chromium_src')

    // Return true when original file of |file| should be touched.
    const applyFileFilter = (file: string) => {
      // Only include overridable files.
      const supportedExts = [
        '.cc',
        '.css',
        '.h',
        '.html',
        '.icon',
        '.json',
        '.mm',
        '.mojom',
        '.pdl',
        '.py',
        '.ts',
        '.xml',
      ]
      return supportedExts.includes(path.extname(file))
    }

    const chromiumSrcDir = path.join(config.srcDir, 'brave', 'chromium_src')
    const sourceFiles = util.walkSync(chromiumSrcDir, applyFileFilter)
    const additionalGen = getAdditionalGenLocation()

    // Touch original files by updating mtime.
    let isDirty = false
    const chromiumSrcDirLen = chromiumSrcDir.length
    sourceFiles.forEach((chromiumSrcFile) => {
      const relativeChromiumSrcFile = chromiumSrcFile.slice(chromiumSrcDirLen)
      let overriddenFile = path.join(config.srcDir, relativeChromiumSrcFile)

      const additionalExtensions = [
        // .lit_mangler.ts files are used to modify the upstream .html.ts file at
        // build time.
        '.lit_mangler.ts',
      ]

      const additionalExtension = additionalExtensions.find((key) =>
        overriddenFile.endsWith(key),
      )
      if (additionalExtension) {
        overriddenFile = overriddenFile.substring(
          0,
          overriddenFile.length - additionalExtension.length,
        )
      }

      if (fs.existsSync(overriddenFile)) {
        // If overriddenFile is older than file in chromium_src, touch it to trigger rebuild.
        isDirty =
          updateFileUTimesIfOverrideIsNewer(overriddenFile, chromiumSrcFile)
          || isDirty
      } else {
        // If the original file doesn't exist, assume that it's in the gen dir.
        overriddenFile = path.join(
          config.outputDir,
          'gen',
          relativeChromiumSrcFile,
        )
        isDirty =
          deleteFileIfOverrideIsNewer(overriddenFile, chromiumSrcFile)
          || isDirty
        // Also check the secondary gen dir, if exists
        if (additionalGen) {
          overriddenFile = path.join(
            config.outputDir,
            additionalGen,
            'gen',
            relativeChromiumSrcFile,
          )
          isDirty =
            deleteFileIfOverrideIsNewer(overriddenFile, chromiumSrcFile)
            || isDirty
        }
      }
    })
    if (isDirty && config.rbeService) {
      // Cleanup Reproxy deps cache on chromium_src override change.
      const reproxyCacheDir = `${config.rootDir}/.reproxy_cache`
      if (fs.existsSync(reproxyCacheDir)) {
        const cacheFileFilter = (file: string) => {
          return file.endsWith('.cache') || file.endsWith('.cache.sha256')
        }
        for (const file of util.walkSync(reproxyCacheDir, cacheFileFilter)) {
          fs.rmSync(file)
        }
      }
    }
    Log.progressFinish('touch original files overridden by chromium_src')
  },

  mergeWithDefault: (options: RunOptions) => {
    return Object.assign({}, config.defaultOptions, options)
  },

  runGnGen: (
    outputDir: string,
    buildArgs: Record<string, unknown>,
    extraGnGenOpts: string[] = [],
    options: RunOptions = config.defaultOptions,
  ) => {
    // Store extraGnGenOpts in buildArgs as a comment to rerun gn gen on change.
    assert(Array.isArray(extraGnGenOpts))
    if (extraGnGenOpts.length) {
      buildArgs[`# Extra gn gen options: ${extraGnGenOpts.join(' ')}`] = null
    }

    // Guard to check if gn gen was successful last time.
    const gnGenGuard = new ActionGuard(path.join(outputDir, 'gn_gen.guard'))

    gnGenGuard.run((wasInterrupted) => {
      const doesBuildNinjaExist = fs.existsSync(
        path.join(outputDir, 'build.ninja'),
      )
      const hasBuildArgsUpdated = util.writeGnBuildArgs(outputDir, buildArgs)
      const shouldCheck = isCI
      const internalOpts = shouldCheck ? ['--check'] : []

      const shouldRunGnGen =
        config.force_gn_gen
        || !doesBuildNinjaExist
        || hasBuildArgsUpdated
        || shouldCheck
        || wasInterrupted

      if (shouldRunGnGen) {
        util.run(
          'gn',
          ['gen', outputDir, ...extraGnGenOpts, ...internalOpts],
          options,
        )
      }
    })
  },

  writeGnBuildArgs: (outputDir: string, buildArgs: Record<string, unknown>) => {
    // Generate build arguments in .gni format to be imported into args.gn. This
    // approach enables customization of args.gn without the build scripts
    // resetting it during each execution.
    const generatedArgsContent = [
      '# Do not edit, any changes will be lost on next build.',
      '# To customize build args, use args.gn in the same directory.\n',
      ...Object.entries(buildArgs)
        // undefined values filtered out to allow gn to use default values in
        // the absence of an .env key.
        .filter(([_, val]) => val !== undefined)
        .map(([arg, val]) => {
          assert(typeof arg === 'string')
          if (val === null) {
            // Output only arg, it may be a comment or an import statement.
            return arg
          }
          return `${arg}=${JSON.stringify(val)}`
        }),
    ].join('\n')

    // Write the generated arguments to the args_generated.gni file. The file
    // name is intentionally chosen to be close to args.gn.
    const generatedArgsFilePath = path.join(outputDir, 'args_generated.gni')
    const hasGeneratedArgsUpdated = util.writeFileIfModified(
      generatedArgsFilePath,
      generatedArgsContent + '\n',
    )
    if (hasGeneratedArgsUpdated) {
      Log.status(`${generatedArgsFilePath} has been updated`)
    }

    // Import args_generated.gni into args.gn.
    const argsGnFilePath = path.join(outputDir, 'args.gn')
    const generatedArgsImportLine = `import("//${path
      .relative(config.srcDir, generatedArgsFilePath)
      .replace(/\\/g, '/')}")`

    // Check if the import statement from args_generated.gni is present in
    // args.gn, even if the user has made modifications. This import statement
    // can also be commented out, allowing the user to fully ignore generated
    // arguments.
    fs.ensureFileSync(argsGnFilePath)
    const isArgsGnValid = fs
      .readFileSync(argsGnFilePath, { encoding: 'utf-8' })
      .includes(generatedArgsImportLine)

    if (!isArgsGnValid) {
      const argsGnContent = [
        "# This file is user-editable. It won't be overwritten as long as it imports",
        '# args_generated.gni, even if the import statement is commented out.\n',
        generatedArgsImportLine,
        '',
        '# Put your extra args AFTER this line.',
      ].join('\n')
      fs.writeFileSync(argsGnFilePath, argsGnContent + '\n')
      Log.status(`${argsGnFilePath} has been updated`)
    }

    return hasGeneratedArgsUpdated || !isArgsGnValid
  },

  generateNinjaFiles: async (options: RunOptions = config.defaultOptions) => {
    await Log.progressScopeAsync('generate ninja files', async () => {
      const extraGnGenOpts = config.extraGnGenOpts
        ? [config.extraGnGenOpts]
        : []
      util.runGnGen(
        config.outputDir,
        getBuildArgs(config),
        extraGnGenOpts,
        options,
      )
    })
  },

  buildTargets: async (
    targets: string[] = config.buildTargets,
    options: RunAsyncOptions = config.defaultOptions,
  ) => {
    assert(Array.isArray(targets))

    if (config.use_clang_coverage) {
      const instrumentationFile = path.join(
        config.outputDir,
        'files-to-instrument.txt',
      )
      await generateInstrumentationFile(instrumentationFile)
    }

    const buildId = crypto.randomUUID()
    const outputDir = config.outputDir
    const progressMessage = `build ${targets} (${path.basename(
      outputDir,
    )}, id=${buildId})`
    Log.progressStart(progressMessage)

    let numCompileFailure = 1
    if (config.ignore_compile_failure) numCompileFailure = 0

    const ninjaOpts: string[] = [
      '-C',
      outputDir,
      ...targets,
      '-k',
      String(numCompileFailure),
      ...config.extraNinjaOpts,
    ]

    // Setting `AUTONINJA_BUILD_ID` allows tracing remote execution which helps
    // with debugging issues (e.g., slowness or remote-failures).
    options.env = {
      ...(options.env ?? process.env),
      AUTONINJA_BUILD_ID: buildId,
    }

    // Collect build statistics into this variable to display in a separate TC
    // block.
    let buildStats = ''
    // Updated on every autoninja log line when CI pipes output (idle watchdog).
    let lastBuildLogTime = Date.now()

    // Parse output to display the build progress on Teamcity.
    if (isTeamcity) {
      let lastStatusTime = Date.now()
      options.onStdOutLine = (line: string) => {
        lastBuildLogTime = Date.now()
        if (
          buildStats
          || /^(RBE Stats:|metric\s+count|build finished)\s+/.test(line)
        ) {
          buildStats += line + '\n'
        } else {
          console.log(line)
          if (Date.now() - lastStatusTime > 5000) {
            // Extract the status message from the siso output.
            const match = line.match(/^\[(.+?)\]/)
            if (match) {
              lastStatusTime = Date.now()
              Log.status(`build ${targets} ${match[1]}`)
            }
          }
        }
      }
      options.onStdErrLine = options.onStdOutLine
      options.stdio = 'pipe'
    } else if (isCI) {
      const onLine = (line: string) => {
        lastBuildLogTime = Date.now()
        console.log(line)
      }
      options.onStdOutLine = onLine
      options.onStdErrLine = onLine
      options.stdio = 'pipe'
    }

    // Enable to allow error post-processing after autoninja/siso failure.
    options.continueOnFail = true

    // Ensure siso_output doesn't exist before the build.
    const sisoOutputFile = path.join(outputDir, 'siso_output')
    if (fs.existsSync(sisoOutputFile)) {
      fs.unlinkSync(sisoOutputFile)
    }

    let buildIdleWatchdogInterval: NodeJS.Timeout | null = null
    const clearBuildIdleWatchdog = () => {
      if (buildIdleWatchdogInterval) {
        clearInterval(buildIdleWatchdogInterval)
        buildIdleWatchdogInterval = null
      }
    }

    const buildGuard = new ActionGuard(path.join(outputDir, 'build.guard'))
    try {
      if (
        isCI
        // Release builds can have steps that can be interrupted by timeouts. We
        // don't want to clean the build in this case.
        && !config.isBraveReleaseBuild()
        && buildGuard.wasInterrupted()
      ) {
        await util.runAsync('gn', ['clean', outputDir], options)
      }
      buildGuard.markStarted()

      let buildProcess: ChildProcess | null = null
      const autoninjaOptions = {
        ...options,
        onSpawn: (prog: ChildProcess) => {
          buildProcess = prog
        },
      }

      if (isCI) {
        const idleTimeoutMs = 60 * 60 * 1000 // 60 minutes
        lastBuildLogTime = Date.now()
        buildIdleWatchdogInterval = setInterval(() => {
          if (Date.now() - lastBuildLogTime <= idleTimeoutMs) {
            return
          }
          clearBuildIdleWatchdog()
          Log.error(
            `Build aborted: no autoninja output for ${idleTimeoutMs / 1000}s `,
          )
          buildDiagnostics.dumpBuildHangDiagnostics(outputDir)
          buildDiagnostics.dumpProcessHangDiagnostics()
          processUtil.killProcessTree(buildProcess)
        }, 10 * 1000)
      }

      await util.runAsync('autoninja', ninjaOpts, autoninjaOptions)
      clearBuildIdleWatchdog()
      buildGuard.markFinished()
    } catch (e) {
      clearBuildIdleWatchdog()
      // Display siso_output on CI after a build failure.
      if (isCI && fs.existsSync(sisoOutputFile)) {
        const sisoOutput = fs.readFileSync(sisoOutputFile, 'utf8')
        Log.error(`Siso output from ${sisoOutputFile}:`)
        // Split the output into lines to correctly display on Teamcity.
        const lines = sisoOutput.split('\n')
        // Output starting from the first "FAILED:" line, or full file.
        const failedIndex = lines.findIndex((line) =>
          line.startsWith('FAILED:'),
        )
        const startIndex = failedIndex !== -1 ? failedIndex : 0
        for (let i = startIndex; i < lines.length; i++) {
          Log.error(lines[i])
        }
      }
      console.error(e.message)
      const exitCode = e.statusCode || 1
      // If the build failed due to an expected build error, mark the build as
      // not interrupted.
      if (exitCode === 1) {
        buildGuard.markFinished()
      }
      process.exit(exitCode)
    }

    Log.progressFinish(progressMessage)

    if (isTeamcity) {
      if (buildStats) {
        Log.progressScope('report build stats', () => {
          console.log(buildStats)
        })
      }
      const sisoExplainFile = path.join(outputDir, 'siso_explain')
      if (fs.existsSync(sisoExplainFile)) {
        const lineLimit = 20
        await Log.progressScopeAsync(
          `siso explain (first ${lineLimit} lines)`,
          async () => {
            for await (const line of util.readLines(
              sisoExplainFile,
              lineLimit,
            )) {
              console.log(line)
            }
          },
        )
      }
    }
  },

  generateXcodeWorkspace: () => {
    console.log(
      'generating Xcode workspace for "' + config.xcode_gen_target + '"...',
    )

    const genScript = path.join(
      config.braveCoreDir,
      'vendor',
      'gn-project-generators',
      'xcode.py',
    )

    const genArgs = [
      '--ide=json',
      '--json-ide-script="' + genScript + '"',
      '--filters="' + config.xcode_gen_target + '"',
    ]

    util.runGnGen(config.outputDir + '_Xcode', getBuildArgs(config), genArgs)
  },

  // Get the files that have been changed in the current diff with base branch.
  getChangedFiles: (repoDir: string, base: string, skipLogging = false) => {
    const upstreamCommit = util
      .run('git', ['merge-base', 'HEAD', base], { cwd: repoDir, skipLogging })
      .stdout.toString()
      .trim()

    return util
      .run('git', ['diff', '--name-only', '--diff-filter=d', upstreamCommit], {
        cwd: repoDir,
        skipLogging,
      })
      .stdout.toString()
      .trim()
      .split('\n')
  },

  massRename: () => {
    const cmdOptions = config.defaultOptions
    cmdOptions.cwd = config.braveCoreDir
    util.run(
      'python3',
      [path.join(config.srcDir, 'tools', 'git', 'mass-rename.py')],
      cmdOptions,
    )
  },

  runGclient: (
    args: string[],
    options: RunOptions = {},
    gclientFile = config.gclientFile,
  ) => {
    if (config.gclientVerbose) {
      args.push('--verbose')
    }
    options.cwd = options.cwd || config.rootDir
    options = util.mergeWithDefault(options)
    options.env ??= {}
    options.env.GCLIENT_FILE = gclientFile
    util.run('gclient', args, options)
  },

  applyPatches: (printPatchFailuresInJson?: boolean) => {
    return applyPatches(printPatchFailuresInJson)
  },

  walkSync: (
    dir: string,
    filter: ((file: string) => boolean) | null = null,
    filelist: string[] = [],
  ): string[] => {
    fs.readdirSync(dir).forEach((file) => {
      if (fs.statSync(path.join(dir, file)).isDirectory()) {
        filelist = util.walkSync(path.join(dir, file), filter, filelist)
      } else if (!filter || filter(file)) {
        filelist = filelist.concat(path.join(dir, file))
      }
    })
    return filelist
  },

  appendExeIfWin32: (input: string) => {
    if (process.platform === 'win32') input += '.exe'
    return input
  },

  readJSON: (file: string, defaultValue: any = undefined) => {
    if (!fs.existsSync(file)) {
      return defaultValue
    }
    try {
      return fs.readJSONSync(file)
    } catch {
      return defaultValue
    }
  },

  writeJSON: (file: string, value: unknown) => {
    return fs.writeJSONSync(file, value, { spaces: 2 })
  },

  getGitDir: (repoDir: string) => {
    const dotGitPath = path.join(repoDir, '.git')
    if (!fs.existsSync(dotGitPath)) {
      return null
    }
    if (fs.statSync(dotGitPath).isDirectory()) {
      return dotGitPath
    }
    // Returns the actual .git dir in case a worktree is used.
    const gitDir = util.runGit(
      repoDir,
      ['rev-parse', '--git-common-dir'],
      false,
    )
    if (!gitDir) {
      return null
    }
    if (!path.isAbsolute(gitDir)) {
      return path.join(repoDir, gitDir)
    }
    return gitDir
  },

  getGitInfoExcludeFileName: (repoDir: string, create: boolean) => {
    const gitDir = util.getGitDir(repoDir)
    if (!gitDir) {
      assert(!create, `Can't create git exclude, .git not found in: ${repoDir}`)
      return null
    }
    const gitInfoDir = path.join(gitDir, 'info')
    const excludeFileName = path.join(gitInfoDir, 'exclude')
    if (!fs.existsSync(excludeFileName)) {
      if (!create) {
        return null
      }
      if (!fs.existsSync(gitInfoDir)) {
        fs.mkdirSync(gitInfoDir)
      }
      fs.writeFileSync(excludeFileName, '')
    }
    return excludeFileName
  },

  isGitExclusionExists: (repoDir: string, exclusion: string) => {
    const excludeFileName = util.getGitInfoExcludeFileName(repoDir, false)
    if (!excludeFileName) {
      return false
    }
    const lines = fs.readFileSync(excludeFileName, 'utf8').split(/\r?\n/)
    return lines.includes(exclusion)
  },

  modifyGitExclusions: (
    repoDir: string,
    { add = [], remove = [] }: GitExclusions,
  ) => {
    const excludeFileName = util.getGitInfoExcludeFileName(
      repoDir,
      add.length > 0,
    )
    if (!excludeFileName) {
      return
    }
    let lines = fs.readFileSync(excludeFileName, 'utf8').split(/\r?\n/)
    lines = lines.filter((line) => !remove.includes(line))
    for (const exclusion of add) {
      if (!lines.includes(exclusion)) {
        lines.push(exclusion)
      }
    }
    util.writeFileIfModified(excludeFileName, lines.join('\n'))
  },

  fetchAndCheckoutRef: (repoDir: string, ref: string) => {
    const options: RunOptions = { cwd: repoDir, stdio: 'inherit' }
    util.run('git', ['fetch', 'origin', ref.replace(/^origin\//, '')], options)
    util.run(
      'git',
      ['-c', 'advice.detachedHead=false', 'checkout', 'FETCH_HEAD'],
      options,
    )
  },

  writeFileIfModified: (filePath: string, content: string) => {
    if (
      !fs.existsSync(filePath)
      || fs.readFileSync(filePath, { encoding: 'utf-8' }) !== content
    ) {
      // Write file atomically.
      const tmpFilePath = `${filePath}.tmp`
      fs.writeFileSync(tmpFilePath, content)
      fs.renameSync(tmpFilePath, filePath)
      return true
    }
    return false
  },

  readLines: async function* (filePath: string, maxLines?: number) {
    const rl = readline.createInterface({
      input: fs.createReadStream(filePath),
      crlfDelay: Infinity,
    })

    let lineCount = 0
    for await (const line of rl) {
      if (maxLines && lineCount >= maxLines) {
        rl.close()
        break
      }
      yield line
      lineCount++
    }
  },

  launchDocs: () => {
    util.run(
      'vpython3',
      [
        path.join(config.srcDir, 'tools', 'md_browser', 'md_browser.py'),
        'brave/docs',
      ],
      config.defaultOptions,
    )
  },
}

export default util
