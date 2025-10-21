// Copyright (c) 2016 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const { spawn, spawnSync } = require('child_process')
const readline = require('readline')
const os = require('os')
const config = require('./config')
const fs = require('fs-extra')
const {glob, writeFile} = require('fs/promises')
const crypto = require('crypto')
const Log = require('./logging')
const assert = require('assert')
const updateChromeVersion = require('./updateChromeVersion')
const ActionGuard = require('./actionGuard')

// Do not limit the number of listeners to avoid warnings from EventEmitter.
process.setMaxListeners(0)

async function generateInstrumentationFile(instrumentationFile) {
  const files = await Array.fromAsync(glob(`**/*.{cc,c,h,cpp,hpp,m,mm}`))

  const paths = files.map(x => `../../brave/${x}`)
  await fs.mkdirp(path.dirname(instrumentationFile));
  await writeFile(instrumentationFile, paths.join('\n'), 'utf-8')
}

async function applyPatches(printPatchFailuresInJson) {
  const GitPatcher = require('./gitPatcher')
  Log.progressStart('apply patches')
  // Always detect if we need to apply patches, since user may have modified
  // either chromium source files, or .patch files manually
  const coreRepoPath = config.braveCoreDir
  const patchesPath = path.join(coreRepoPath, 'patches')
  const v8PatchesPath = path.join(patchesPath, 'v8')
  const catapultPatchesPath = path.join(patchesPath, 'third_party', 'catapult')
  const devtoolsFrontendPatchesPath = path.join(
    patchesPath,
    'third_party',
    'devtools-frontend',
    'src',
  )
  const searchEngineDataPatchesPath = path.join(
    patchesPath,
    'third_party',
    'search_engines_data',
    'resources',
  )

  const chromiumRepoPath = config.srcDir
  const v8RepoPath = path.join(chromiumRepoPath, 'v8')
  const catapultRepoPath = path.join(
    chromiumRepoPath,
    'third_party',
    'catapult',
  )
  const devtoolsFrontendRepoPath = path.join(
    chromiumRepoPath,
    'third_party',
    'devtools-frontend',
    'src',
  )
  const searchEngineDataRepoPath = path.join(
    chromiumRepoPath,
    'third_party',
    'search_engines_data',
    'resources',
  )

  const chromiumPatcher = new GitPatcher(patchesPath, chromiumRepoPath)
  const v8Patcher = new GitPatcher(v8PatchesPath, v8RepoPath)
  const catapultPatcher = new GitPatcher(catapultPatchesPath, catapultRepoPath)
  const devtoolsFrontendPatcher = new GitPatcher(
    devtoolsFrontendPatchesPath,
    devtoolsFrontendRepoPath,
  )
  const searchEngineDataPatcher = new GitPatcher(
    searchEngineDataPatchesPath,
    searchEngineDataRepoPath,
  )

  const chromiumPatchStatus = await chromiumPatcher.applyPatches()
  const v8PatchStatus = await v8Patcher.applyPatches()
  const catapultPatchStatus = await catapultPatcher.applyPatches()
  const devtoolsFrontendPatchStatus =
    await devtoolsFrontendPatcher.applyPatches()
  const searchEngineDataPatchStatus =
    await searchEngineDataPatcher.applyPatches()

  // Log status for all patches
  // Differentiate entries for logging
  v8PatchStatus.forEach((s) => (s.path = path.join('v8', s.path)))
  catapultPatchStatus.forEach(
    (s) => (s.path = path.join('third_party', 'catapult', s.path)),
  )
  devtoolsFrontendPatchStatus.forEach(
    (s) =>
      (s.path = path.join('third_party', 'devtools-frontend', 'src', s.path)),
  )
  const allPatchStatus = [
    ...chromiumPatchStatus,
    ...v8PatchStatus,
    ...catapultPatchStatus,
    ...devtoolsFrontendPatchStatus,
    ...searchEngineDataPatchStatus,
  ]
  if (printPatchFailuresInJson) {
    Log.printFailedPatchesInJsonFormat(allPatchStatus, config.braveCoreDir)
  } else {
    Log.allPatchStatus(allPatchStatus, 'Chromium')
  }

  const hasPatchError = allPatchStatus.some((p) => p.error)
  // Exit on error in any patch
  if (hasPatchError) {
    Log.error('Exiting as not all patches were successful!')
    process.exit(1)
  }

  updateChromeVersion()
  Log.progressFinish('apply patches')
}

const isOverrideNewer = (original, override) => {
  return fs.statSync(override).mtimeMs - fs.statSync(original).mtimeMs > 0
}

const updateFileUTimesIfOverrideIsNewer = (original, override) => {
  if (isOverrideNewer(original, override)) {
    const date = new Date()
    fs.utimesSync(original, date, date)
    console.log(original + ' is touched.')
    return true
  }
  return false
}

const deleteFileIfOverrideIsNewer = (original, override) => {
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

const getAdditionalGenLocation = () => {
  if (config.targetOS === 'android') {
    if (config.targetArch === 'arm64') {
      return 'android_clang_arm'
    } else if (config.targetArch === 'x64') {
      return 'android_clang_x86'
    }
  } else if (
    (process.platform === 'darwin' || process.platform === 'linux') &&
    config.targetArch === 'arm64'
  ) {
    return 'clang_x64_v8_arm64'
  }
  return ''
}

const normalizeCommand = (cmd, args) => {
  if (process.platform === 'win32') {
    args = ['/c', cmd, ...args]
    cmd = 'cmd'
  }
  return [ cmd, args ]
}

const util = {
  generateInstrumentationFile,
  runProcess: (cmd, args = [], options = {}, skipLogging = false) => {
    if (!skipLogging) {
      Log.command(options.cwd, cmd, args)
    }
    return spawnSync(...normalizeCommand(cmd, args), options)
  },

  run: (cmd, args = [], options = {}) => {
    const { continueOnFail, skipLogging, logError, ...cmdOptions } = options
    const prog = util.runProcess(cmd, args, cmdOptions, skipLogging)
    if (prog.status !== 0) {
      if (!continueOnFail || logError) {
        if (skipLogging) {
          console.log(cmd, args, 'exited with status', prog.status, cmdOptions)
        }

        console.log(prog.stdout && prog.stdout.toString())
        console.error(prog.stderr && prog.stderr.toString())
      }
      if (!continueOnFail) {
        process.exit(1)
      }
    }
    return prog
  },

  runGit: (repoPath, gitArgs, continueOnFail = false, options = {}) => {
    let prog = util.run('git', gitArgs, { cwd: repoPath, continueOnFail, ...options})

    if (prog.status !== 0) {
      return null
    } else {
      return prog.stdout.toString().trim()
    }
  },

  runAsync: (cmd, args = [], options = {}) => {
    let { continueOnFail, verbose, onStdErrLine, onStdOutLine, ...cmdOptions } =
      options
    if (verbose !== false) {
      Log.command(cmdOptions.cwd, cmd, args)
    }
    return new Promise((resolve, reject) => {
      const prog = spawn(...normalizeCommand(cmd, args), cmdOptions)
      const signalsToForward = ['SIGINT', 'SIGTERM', 'SIGQUIT', 'SIGHUP']
      const signalHandler = (s) => {
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
      const closeHandler = (statusCode, signal) => {
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
          const err = new Error(
            `Program ${cmd} exited with error code ${statusCode}.`,
          )
          err.stderr = stderr
          err.stdout = stdout
          err.statusCode = statusCode
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
        if (config.isCI && (statusCode || signal)) {
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

  runGitAsync: function (repoPath, gitArgs, verbose = false, logError = false) {
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

  getGitReadableLocalRef: (repoDir) => {
    return util.runGit(
      repoDir,
      ['log', '-n', '1', '--pretty=format:%h%d'],
      true,
    )
  },

  calculateFileChecksum: (filename) => {
    // adapted from https://github.com/kodie/md5-file
    const BUFFER_SIZE = 8192
    const fd = fs.openSync(filename, 'r')
    const buffer = Buffer.alloc(BUFFER_SIZE)
    const md5 = crypto.createHash('md5')

    try {
      let bytesRead
      do {
        bytesRead = fs.readSync(fd, buffer, 0, BUFFER_SIZE)
        md5.update(buffer.slice(0, bytesRead))
      } while (bytesRead === BUFFER_SIZE)
    } finally {
      fs.closeSync(fd)
    }

    return md5.digest('hex')
  },

  touchOverriddenFiles: () => {
    Log.progressStart('touch original files overridden by chromium_src')

    // Return true when original file of |file| should be touched.
    const applyFileFilter = (file) => {
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
        isDirty |= updateFileUTimesIfOverrideIsNewer(
          overriddenFile,
          chromiumSrcFile,
        )
      } else {
        // If the original file doesn't exist, assume that it's in the gen dir.
        overriddenFile = path.join(
          config.outputDir,
          'gen',
          relativeChromiumSrcFile,
        )
        isDirty |= deleteFileIfOverrideIsNewer(overriddenFile, chromiumSrcFile)
        // Also check the secondary gen dir, if exists
        if (additionalGen) {
          overriddenFile = path.join(
            config.outputDir,
            additionalGen,
            'gen',
            relativeChromiumSrcFile,
          )
          isDirty |= deleteFileIfOverrideIsNewer(
            overriddenFile,
            chromiumSrcFile,
          )
        }
      }
    })
    if (isDirty && config.rbeService) {
      // Cleanup Reproxy deps cache on chromium_src override change.
      const reproxyCacheDir = `${config.rootDir}/.reproxy_cache`
      if (fs.existsSync(reproxyCacheDir)) {
        const cacheFileFilter = (file) => {
          return file.endsWith('.cache') || file.endsWith('.cache.sha256')
        }
        for (const file of util.walkSync(reproxyCacheDir, cacheFileFilter)) {
          fs.rmSync(file)
        }
      }
    }
    Log.progressFinish('touch original files overridden by chromium_src')
  },

  touchGsutilChangeLogFile: () => {
    // Chromium team confirmed that ChangeLog file was likely removed by accident
    // https://chromium-review.googlesource.com/c/catapult/+/4567074?tab=comments

    // However this is just a temp solution. This file is not
    // used in Chromium tests, so eventually we should find out what is the
    // difference in the way we run the tests. Follow up issue
    // https://github.com/brave/brave-browser/issues/31641
    console.log('touch gsutil ChangeLog file...')

    const changeLogFile = path.join(
      config.srcDir,
      'third_party',
      'catapult',
      'third_party',
      'gsutil',
      'third_party',
      'mock',
      'ChangeLog',
    )
    if (!fs.existsSync(changeLogFile)) {
      fs.writeFileSync(changeLogFile, '')
    }
  },

  mergeWithDefault: (options) => {
    return Object.assign({}, config.defaultOptions, options)
  },

  buildNativeRedirectCC: async () => {
    if (config.useSiso) {
      // redirect_cc logic is handled by siso handler.
      return
    }

    // Expected path to redirect_cc.
    const redirectCC = path.join(
      config.nativeRedirectCCDir,
      util.appendExeIfWin32('redirect_cc'),
    )

    // Only build if the source has changed.
    if (
      fs.existsSync(redirectCC) &&
      fs.statSync(redirectCC).mtime >=
        fs.statSync(
          path.join(
            config.braveCoreDir,
            'tools',
            'redirect_cc',
            'redirect_cc.cc',
          ),
        ).mtime
    ) {
      return
    }

    Log.progressStart('build redirect_cc')
    const buildArgs = {
      'import("//brave/tools/redirect_cc/args.gni")': null,
      use_remoteexec: config.useRemoteExec,
      use_reclient: config.useRemoteExec,
      use_siso: false,
      reclient_bin_dir: config.realRewrapperDir,
      real_rewrapper: path.join(config.realRewrapperDir, 'rewrapper'),
    }

    util.runGnGen(config.nativeRedirectCCDir, buildArgs, [
      '--root-target=//brave/tools/redirect_cc',
    ])
    await util.buildTargets(
      ['brave/tools/redirect_cc'],
      util.mergeWithDefault({ outputDir: config.nativeRedirectCCDir }),
    )
    Log.progressFinish('build redirect_cc')
  },

  runGnGen: (
    outputDir,
    buildArgs,
    extraGnGenOpts = [],
    options = config.defaultOptions,
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
      const shouldCheck = config.isCI
      const internalOpts = shouldCheck ? ['--check'] : []

      const shouldRunGnGen =
        config.force_gn_gen ||
        !doesBuildNinjaExist ||
        hasBuildArgsUpdated ||
        shouldCheck ||
        wasInterrupted

      if (shouldRunGnGen) {
        util.run(
          'gn',
          ['gen', outputDir, ...extraGnGenOpts, ...internalOpts],
          options,
        )
      }
    })
  },

  writeGnBuildArgs: (outputDir, buildArgs) => {
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

  generateNinjaFiles: async (options = config.defaultOptions) => {
    await Log.progressScopeAsync('generate ninja files', async () => {
      await util.buildNativeRedirectCC()

      const extraGnGenOpts = config.extraGnGenOpts
        ? [config.extraGnGenOpts]
        : []
      util.runGnGen(
        config.outputDir,
        config.buildArgs(),
        extraGnGenOpts,
        options,
      )
    })
  },

  buildTargets: async (
    targets = config.buildTargets,
    options = config.defaultOptions,
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
    const outputDir = options.outputDir || config.outputDir
    const progressMessage = `build ${targets} (${path.basename(
      outputDir,
    )}, id=${buildId})`
    Log.progressStart(progressMessage)

    let numCompileFailure = 1
    if (config.ignore_compile_failure) numCompileFailure = 0

    let ninjaOpts = [
      '-C',
      outputDir,
      ...targets,
      '-k',
      numCompileFailure,
      ...config.extraNinjaOpts,
    ]

    // Setting `AUTONINJA_BUILD_ID` allows tracing remote execution which helps
    // with debugging issues (e.g., slowness or remote-failures).
    options.env.AUTONINJA_BUILD_ID = buildId

    // Collect build statistics into this variable to display in a separate TC
    // block.
    let buildStats = ''

    // Parse output to display the build progress on Teamcity.
    if (config.isTeamcity) {
      let lastStatusTime = Date.now()
      options.onStdOutLine = (line) => {
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
    }

    // Enable to allow error post-processing after autoninja/siso failure.
    options.continueOnFail = true

    // Ensure siso_output doesn't exist before the build.
    const sisoOutputFile = path.join(outputDir, 'siso_output')
    if (fs.existsSync(sisoOutputFile)) {
      fs.unlinkSync(sisoOutputFile)
    }

    const buildGuard = new ActionGuard(path.join(outputDir, 'build.guard'))
    try {
      if (
        config.isCI &&
        // Release builds can have steps that can be interrupted by timeouts. We
        // don't want to clean the build in this case.
        !config.isBraveReleaseBuild() &&
        buildGuard.wasInterrupted()
      ) {
        await util.runAsync('gn', ['clean', outputDir], options)
      }
      buildGuard.markStarted()
      await util.runAsync('autoninja', ninjaOpts, options)
      buildGuard.markFinished()
    } catch (e) {
      // Display siso_output on CI after a build failure.
      if (config.isCI && fs.existsSync(sisoOutputFile)) {
        const sisoOutput = fs.readFileSync(sisoOutputFile, 'utf8')
        Log.error(`Siso output from ${sisoOutputFile}:`)
        // Split the output into lines to correctly display on Teamcity.
        for (const line of sisoOutput.split('\n')) {
          Log.error(line)
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

    if (config.isTeamcity && buildStats) {
      Log.progressScope('report build stats', () => {
        console.log(buildStats)
      })
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

    util.runGnGen(config.outputDir + '_Xcode', config.buildArgs(), genArgs)
  },

  // Get the files that have been changed in the current diff with base branch.
  getChangedFiles: (repoDir, base, skipLogging = false) => {
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

  presubmit: (options = {}) => {
    if (!options.base) {
      options.base = 'origin/master'
    }
    // Temporary cleanup call, should be removed when everyone will remove
    // 'gerrit.host' from their brave checkout.
    util.runGit(
      config.braveCoreDir,
      ['config', '--unset-all', 'gerrit.host'],
      true,
    )
    let cmdOptions = config.defaultOptions
    cmdOptions.cwd = config.braveCoreDir
    cmdOptions = util.mergeWithDefault(cmdOptions)
    cmd = 'git'
    // --upload mode is similar to `git cl upload`. Non-upload mode covers less
    // checks.
    args = ['cl', 'presubmit', options.base, '--force', '--upload']
    if (options.all) args.push('--all')
    if (options.files) args.push('--files', `"${options.files}"`)
    if (options.verbose) {
      args.push(...Array(options.verbose).fill('--verbose'))
    }
    if (options.json) {
      args.push('-j')
      args.push(options.json)
    }

    if (options.fix) {
      cmdOptions.env.PRESUBMIT_FIX = '1'
    }
    util.run(cmd, args, cmdOptions)
  },

  massRename: (options = {}) => {
    let cmdOptions = config.defaultOptions
    cmdOptions.cwd = config.braveCoreDir
    util.run(
      'python3',
      [path.join(config.srcDir, 'tools', 'git', 'mass-rename.py')],
      cmdOptions,
    )
  },

  runGClient: (args, options = {}, gClientFile = config.gClientFile) => {
    if (config.gClientVerbose) {
      args.push('--verbose')
    }
    options.cwd = options.cwd || config.rootDir
    options = util.mergeWithDefault(options)
    options.env.GCLIENT_FILE = gClientFile
    util.run('gclient', args, options)
  },

  applyPatches: (printPatchFailuresInJson) => {
    return applyPatches(printPatchFailuresInJson)
  },

  walkSync: (dir, filter = null, filelist = []) => {
    fs.readdirSync(dir).forEach((file) => {
      if (fs.statSync(path.join(dir, file)).isDirectory()) {
        filelist = util.walkSync(path.join(dir, file), filter, filelist)
      } else if (!filter || filter.call(null, file)) {
        filelist = filelist.concat(path.join(dir, file))
      }
    })
    return filelist
  },

  appendExeIfWin32: (input) => {
    if (process.platform === 'win32') input += '.exe'
    return input
  },

  readJSON: (file, defaultValue = undefined) => {
    if (!fs.existsSync(file)) {
      return defaultValue
    }
    try {
      return fs.readJSONSync(file)
    } catch {
      return defaultValue
    }
  },

  writeJSON: (file, value) => {
    return fs.writeJSONSync(file, value, { spaces: 2 })
  },

  getGitDir: (repoDir) => {
    const dotGitPath = path.join(repoDir, '.git')
    if (!fs.existsSync(dotGitPath)) {
      return null
    }
    if (fs.statSync(dotGitPath).isDirectory()) {
      return dotGitPath
    }
    // Returns the actual .git dir in case a worktree is used.
    gitDir = util.runGit(repoDir, ['rev-parse', '--git-common-dir'], false)
    if (!path.isAbsolute(gitDir)) {
      return path.join(repoDir, gitDir)
    }
    return gitDir
  },

  getGitInfoExcludeFileName: (repoDir, create) => {
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

  isGitExclusionExists: (repoDir, exclusion) => {
    const excludeFileName = util.getGitInfoExcludeFileName(repoDir, false)
    if (!excludeFileName) {
      return false
    }
    const lines = fs.readFileSync(excludeFileName).toString().split(/\r?\n/)
    for (const line of lines) {
      if (line === exclusion) {
        return true
      }
    }
    return false
  },

  addGitExclusion: (repoDir, exclusion) => {
    if (util.isGitExclusionExists(repoDir, exclusion)) {
      return
    }
    const excludeFileName = util.getGitInfoExcludeFileName(repoDir, true)
    fs.appendFileSync(excludeFileName, '\n' + exclusion)
  },

  fetchAndCheckoutRef: (repoDir, ref) => {
    const options = { cwd: repoDir, stdio: 'inherit' }
    util.run('git', ['fetch', 'origin', ref.replace(/^origin\//, '')], options)
    util.run(
      'git',
      ['-c', 'advice.detachedHead=false', 'checkout', 'FETCH_HEAD'],
      options,
    )
  },

  writeFileIfModified: (filePath, content) => {
    fs.ensureFileSync(filePath)
    if (fs.readFileSync(filePath, { encoding: 'utf-8' }) !== content) {
      fs.writeFileSync(filePath, content)
      return true
    }
    return false
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

module.exports = util
