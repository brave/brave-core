// Copyright (c) 2016 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const { spawn, spawnSync } = require('child_process')
const config = require('./config')
const fs = require('fs-extra')
const crypto = require('crypto')
const l10nUtil = require('./l10nUtil')
const Log = require('./logging')
const assert = require('assert')
const updateChromeVersion = require('./updateChromeVersion')

const mergeWithDefault = (options) => {
  return Object.assign({}, config.defaultOptions, options)
}

async function applyPatches() {
  const GitPatcher = require('./gitPatcher')
  Log.progressStart('apply patches')
  // Always detect if we need to apply patches, since user may have modified
  // either chromium source files, or .patch files manually
  const coreRepoPath = config.braveCoreDir
  const patchesPath = path.join(coreRepoPath, 'patches')
  const v8PatchesPath = path.join(patchesPath, 'v8')
  const catapultPatchesPath = path.join(patchesPath, 'third_party', 'catapult')
  const devtoolsFrontendPatchesPath = path.join(patchesPath, 'third_party', 'devtools-frontend', 'src')
  const ffmpegPatchesPath = path.join(patchesPath, 'third_party', 'ffmpeg')

  const chromiumRepoPath = config.srcDir
  const v8RepoPath = path.join(chromiumRepoPath, 'v8')
  const catapultRepoPath = path.join(chromiumRepoPath, 'third_party', 'catapult')
  const devtoolsFrontendRepoPath = path.join(chromiumRepoPath, 'third_party', 'devtools-frontend', 'src')
  const ffmpegRepoPath = path.join(chromiumRepoPath, 'third_party', 'ffmpeg')

  const chromiumPatcher = new GitPatcher(patchesPath, chromiumRepoPath)
  const v8Patcher = new GitPatcher(v8PatchesPath, v8RepoPath)
  const catapultPatcher = new GitPatcher(catapultPatchesPath, catapultRepoPath)
  const devtoolsFrontendPatcher = new GitPatcher(devtoolsFrontendPatchesPath, devtoolsFrontendRepoPath)
  const ffmpegPatcher = new GitPatcher(ffmpegPatchesPath, ffmpegRepoPath)

  const chromiumPatchStatus = await chromiumPatcher.applyPatches()
  const v8PatchStatus = await v8Patcher.applyPatches()
  const catapultPatchStatus = await catapultPatcher.applyPatches()
  const devtoolsFrontendPatchStatus = await devtoolsFrontendPatcher.applyPatches()
  const ffmpegPatchStatus = await ffmpegPatcher.applyPatches()

  // Log status for all patches
  // Differentiate entries for logging
  v8PatchStatus.forEach(s => s.path = path.join('v8', s.path))
  catapultPatchStatus.forEach(
    s => s.path = path.join('third_party', 'catapult', s.path))
  devtoolsFrontendPatchStatus.forEach(
    s => s.path = path.join('third_party', 'devtools-frontend', 'src', s.path))
  const allPatchStatus = [...chromiumPatchStatus, ...v8PatchStatus, ...catapultPatchStatus, ...devtoolsFrontendPatchStatus, ...ffmpegPatchStatus]
  Log.allPatchStatus(allPatchStatus, 'Chromium')

  const hasPatchError = allPatchStatus.some(p => p.error)
  // Exit on error in any patch
  if (hasPatchError) {
    Log.error('Exiting as not all patches were successful!')
    process.exit(1)
  }

  updateChromeVersion()
  Log.progressFinish('apply patches')
}

const isOverrideNewer = (original, override) => {
  return (fs.statSync(override).mtimeMs - fs.statSync(original).mtimeMs > 0)
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
  } else if ((process.platform === 'darwin' || process.platform === 'linux') && config.targetArch === 'arm64') {
    return 'clang_x64_v8_arm64'
  }
  return ''
}

const util = {

  runProcess: (cmd, args = [], options = {}) => {
    Log.command(options.cwd, cmd, args)
    return spawnSync(cmd, args, options)
  },

  run: (cmd, args = [], options = {}) => {
    const { continueOnFail, ...cmdOptions } = options
    const prog = util.runProcess(cmd, args, cmdOptions)
    if (prog.status !== 0) {
      if (!continueOnFail) {
        console.log(prog.stdout && prog.stdout.toString())
        console.error(prog.stderr && prog.stderr.toString())
        process.exit(1)
      }
    }
    return prog
  },

  runGit: (repoPath, gitArgs, continueOnFail = false) => {
    let prog = util.run('git', gitArgs, { cwd: repoPath, continueOnFail })

    if (prog.status !== 0) {
      return null
    } else {
      return prog.stdout.toString().trim()
    }
  },

  runAsync: (cmd, args = [], options = {}) => {
    let { continueOnFail, verbose, ...cmdOptions } = options
    if (verbose) {
      Log.command(cmdOptions.cwd, cmd, args)
    }
    return new Promise((resolve, reject) => {
      const prog = spawn(cmd, args, cmdOptions)
      let stderr = ''
      let stdout = ''
      prog.stderr.on('data', data => {
        stderr += data
      })
      prog.stdout.on('data', data => {
        stdout += data
      })
      prog.on('close', statusCode => {
        const hasFailed = statusCode !== 0
        if (verbose && (!hasFailed || continueOnFail)) {
          console.log(stdout)
          if (stderr) {
            console.error(stderr)
          }
        }
        if (hasFailed) {
          const err = new Error(`Program ${cmd} exited with error code ${statusCode}.`)
          err.stderr = stderr
          err.stdout = stdout
          reject(err)
          if (!continueOnFail) {
            console.log(err.message)
            console.log(stdout)
            console.error(stderr)
            process.exit(1)
          }
          return
        }
        resolve(stdout)
      })
    })
  },


  runGitAsync: function (repoPath, gitArgs, verbose = false, logError = false) {
    return util.runAsync('git', gitArgs, { cwd: repoPath, verbose, continueOnFail: true })
      .catch(err => {
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
    return util.runGit(repoDir, ['log', '-n', '1', '--pretty=format:%h%d'], true)
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

  updateBranding: () => {
    Log.progressStart('update branding')
    const chromeComponentsDir = path.join(config.srcDir, 'components')
    const braveComponentsDir = path.join(config.braveCoreDir, 'components')
    const chromeAppDir = path.join(config.srcDir, 'chrome', 'app')
    const braveAppDir = path.join(config.braveCoreDir, 'app')
    const chromeBrowserResourcesDir = path.join(config.srcDir, 'chrome', 'browser', 'resources')
    const braveBrowserResourcesDir = path.join(config.braveCoreDir, 'browser', 'resources')
    const braveAppVectorIconsDir = path.join(config.braveCoreDir, 'components')
    const chromeAndroidJavaStringsTranslationsDir = path.join(config.srcDir, 'chrome', 'browser', 'ui', 'android', 'strings', 'translations')
    const braveAndroidJavaStringsTranslationsDir = path.join(config.braveCoreDir, 'browser', 'ui', 'android', 'strings', 'translations')
    const chromeAndroidTabUiJavaStringsTranslationsDir = path.join(config.srcDir, 'chrome', 'android', 'features', 'tab_ui', 'java', 'strings', 'translations')
    const braveAndroidTabUiJavaStringsTranslationsDir = path.join(config.braveCoreDir, 'android', 'features', 'tab_ui', 'java', 'strings', 'translations')

    let fileMap = new Set();
    const autoGeneratedBraveToChromiumMapping = Object.assign({}, l10nUtil.getAutoGeneratedBraveToChromiumMapping())
    // The following 3 entries we map to the same name, not the chromium equivalent name for copying back
    autoGeneratedBraveToChromiumMapping[path.join(braveAppDir, 'brave_strings.grd')] = path.join(chromeAppDir, 'brave_strings.grd')
    autoGeneratedBraveToChromiumMapping[path.join(braveAppDir, 'settings_brave_strings.grdp')] = path.join(chromeAppDir, 'settings_brave_strings.grdp')
    autoGeneratedBraveToChromiumMapping[path.join(braveComponentsDir, 'components_brave_strings.grd')] = path.join(chromeComponentsDir, 'components_brave_strings.grd')

    Object.entries(autoGeneratedBraveToChromiumMapping).forEach(mapping => fileMap.add(mapping))

    // Copy xtb files for:
    // brave/app/resources/chromium_strings*.xtb
    // brave/app/resources/generated_resoruces*.xtb
    // brave/components/strings/components_chromium_strings*.xtb
    // brave/browser/ui/android/strings/translations/android_chrome_strings*.xtb
    // brave/android/features/tab_ui/java/strings/translations/android_chrome_tab_ui_strings*.xtb
    fileMap.add([path.join(braveAppDir, 'resources'), path.join(chromeAppDir, 'resources')])
    fileMap.add([path.join(braveComponentsDir, 'strings'), path.join(chromeComponentsDir, 'strings')])
    fileMap.add([braveAndroidJavaStringsTranslationsDir, chromeAndroidJavaStringsTranslationsDir])
    fileMap.add([braveAndroidTabUiJavaStringsTranslationsDir, chromeAndroidTabUiJavaStringsTranslationsDir])
    // By overwriting, we don't need to modify some grd files.
    fileMap.add([path.join(braveAppDir, 'theme', 'brave'), path.join(chromeAppDir, 'theme', 'brave')])
    fileMap.add([path.join(braveAppDir, 'theme', 'brave'), path.join(chromeAppDir, 'theme', 'chromium')])
    fileMap.add([path.join(braveAppDir, 'theme', 'default_100_percent', 'brave'), path.join(chromeAppDir, 'theme', 'default_100_percent', 'brave')])
    fileMap.add([path.join(braveAppDir, 'theme', 'default_200_percent', 'brave'), path.join(chromeAppDir, 'theme', 'default_200_percent', 'brave')])
    fileMap.add([path.join(braveAppDir, 'theme', 'default_100_percent', 'brave'), path.join(chromeAppDir, 'theme', 'default_100_percent', 'chromium')])
    fileMap.add([path.join(braveAppDir, 'theme', 'default_200_percent', 'brave'), path.join(chromeAppDir, 'theme', 'default_200_percent', 'chromium')])
    fileMap.add([path.join(braveAppDir, 'theme', 'default_100_percent', 'common'), path.join(chromeAppDir, 'theme', 'default_100_percent', 'common')])
    fileMap.add([path.join(braveAppDir, 'theme', 'default_200_percent', 'common'), path.join(chromeAppDir, 'theme', 'default_200_percent', 'common')])
    fileMap.add([path.join(braveComponentsDir, 'resources', 'default_100_percent'), path.join(chromeComponentsDir, 'resources', 'default_100_percent')])
    fileMap.add([path.join(braveComponentsDir, 'resources', 'default_100_percent', 'brave'), path.join(chromeComponentsDir, 'resources', 'default_100_percent', 'chromium')])
    fileMap.add([path.join(braveComponentsDir, 'resources', 'default_200_percent'), path.join(chromeComponentsDir, 'resources', 'default_200_percent')])
    fileMap.add([path.join(braveComponentsDir, 'resources', 'default_200_percent', 'brave'), path.join(chromeComponentsDir, 'resources', 'default_200_percent', 'chromium')])
    fileMap.add([path.join(braveAppVectorIconsDir, 'vector_icons', 'brave'), path.join(chromeComponentsDir, 'vector_icons', 'brave')])
    // Copy chrome-logo-faded.png for replacing chrome logo of welcome page with brave's on Win8.
    fileMap.add([path.join(braveBrowserResourcesDir, 'chrome-logo-faded.png'), path.join(chromeBrowserResourcesDir, 'chrome-logo-faded.png')])
    fileMap.add([path.join(braveBrowserResourcesDir, 'downloads', 'images', 'incognito_marker.svg'), path.join(chromeBrowserResourcesDir, 'downloads', 'images', 'incognito_marker.svg')])
    fileMap.add([path.join(braveBrowserResourcesDir, 'settings', 'images'), path.join(chromeBrowserResourcesDir, 'settings', 'images')])
    fileMap.add([path.join(braveBrowserResourcesDir, 'signin', 'images'), path.join(chromeBrowserResourcesDir, 'signin', 'images')])
    fileMap.add([
      path.join(
          braveBrowserResourcesDir, 'signin', 'profile_customization',
          'images'),
      path.join(
          chromeBrowserResourcesDir, 'signin', 'profile_customization',
          'images')
    ])
    fileMap.add([path.join(braveBrowserResourcesDir, 'signin', 'profile_picker', 'images'), path.join(chromeBrowserResourcesDir, 'signin', 'profile_picker', 'images')])
    fileMap.add([path.join(braveBrowserResourcesDir, 'side_panel', 'reading_list', 'images'), path.join(chromeBrowserResourcesDir, 'side_panel', 'reading_list', 'images')])

    // Copy to make our ${branding_path_product}_behaviors.cc
    fileMap.add([path.join(config.braveCoreDir, 'chromium_src', 'chrome', 'installer', 'setup', 'brave_behaviors.cc'),
                 path.join(config.srcDir, 'chrome', 'installer', 'setup',
                           'brave_behaviors.cc')])
    // Replace webui CSS to use our fonts.
    fileMap.add([path.join(config.braveCoreDir, 'ui', 'webui', 'resources', 'css', 'text_defaults_md.css'),
                 path.join(config.srcDir, 'ui', 'webui', 'resources', 'css', 'text_defaults_md.css')])
    // Replace chrome dark logo with channel specific brave logo.
    fileMap.add([
      path.join(config.braveCoreDir, 'node_modules', '@brave', 'leo', 'icons',
          config.getBraveLogoIconName()),
      path.join(config.srcDir, 'ui', 'webui', 'resources', 'images',
          'chrome_logo_dark.svg')])

    let explicitSourceFiles = new Set()
    if (config.getTargetOS() === 'mac') {
      // Set proper mac app icon for channel to chrome/app/theme/mac/app.icns.
      // Each channel's app icons are stored in brave/app/theme/$channel/app.icns.
      // With this copying, we don't need to modify chrome/BUILD.gn for this.
      const iconSource = path.join(braveAppDir, 'theme', 'brave', 'mac', config.channel, 'app.icns')
      const iconDest = path.join(chromeAppDir, 'theme', 'brave', 'mac', 'app.icns')
      explicitSourceFiles[iconDest] = iconSource

      // Set proper branding file.
      let branding_file_name = 'BRANDING'
      if (config.channel)
        branding_file_name = branding_file_name + '.' + config.channel
      const brandingSource = path.join(braveAppDir, 'theme', 'brave', branding_file_name)
      const brandingDest = path.join(chromeAppDir, 'theme', 'brave', 'BRANDING')
      explicitSourceFiles[brandingDest] = brandingSource
    }

    for (const [source, output] of fileMap) {
      if (!fs.existsSync(source)) {
        console.warn(`Warning: The following file-system entry was not found for copying contents to a chromium destination: ${source}. Consider removing the entry from the file-map, or investigating whether the correct source code reference is checked out.`)
        continue
      }

      let sourceFiles = []

      // get all the files if source if a directory
      if (fs.statSync(source).isDirectory()) {
        sourceFiles = util.walkSync(source)
      } else {
        sourceFiles = [source]
      }

      for (let sourceFile of sourceFiles) {
        const destinationFile = path.join(output, path.relative(source, sourceFile))
        sourceFile = explicitSourceFiles[destinationFile] || sourceFile
        if (!fs.existsSync(destinationFile) ||
            util.calculateFileChecksum(sourceFile) != util.calculateFileChecksum(destinationFile)) {
          fs.copySync(sourceFile, destinationFile)
          console.log(sourceFile + ' copied to ' + destinationFile)
        }
      }
    }

    if (config.targetOS === 'android') {

      let braveOverwrittenFiles = new Set();
      const removeUnlistedAndroidResources = (braveOverwrittenFiles) => {
        const suspectedDir = path.join(config.srcDir, 'chrome', 'android', 'java', 'res')

        let untrackedChromiumFiles = util.runGit(suspectedDir, ['ls-files', '--others', '--exclude-standard'], true).split('\n')
        let untrackedChromiumPaths = [];
        for (const untrackedChromiumFile of untrackedChromiumFiles) {
          untrackedChromiumPath = path.join(suspectedDir, untrackedChromiumFile)

          if (!fs.statSync(untrackedChromiumPath).isDirectory()) {
            untrackedChromiumPaths.push(untrackedChromiumPath);
          }
        }

        const isChildOf = (child, parent) => {
          const relative = path.relative(parent, child);
          return relative && !relative.startsWith('..') && !path.isAbsolute(relative);
        }

        for (const untrackedChromiumPath of untrackedChromiumPaths) {
          if (isChildOf(untrackedChromiumPath, suspectedDir) && !braveOverwrittenFiles.has(untrackedChromiumPath)) {
            fs.removeSync(untrackedChromiumPath);
            console.log(`Deleted not listed file: ${untrackedChromiumPath}`);
          }
        }
      }

      let androidIconSet = ''
      if (config.channel === 'development') {
        androidIconSet = 'res_brave_default'
      }
      else if (config.channel === '') {
        androidIconSet = 'res_brave'
      } else if (config.channel === 'beta') {
        androidIconSet = 'res_brave_beta'
      } else if (config.channel === 'dev') {
        androidIconSet = 'res_brave_dev'
      } else if (config.channel === 'nightly') {
        androidIconSet = 'res_brave_nightly'
      }

      const androidTranslateResSource = path.join(config.braveCoreDir, 'components', 'translate','content' , 'android', 'java', 'res')
      const androidTranslateResDest = path.join(config.srcDir, 'components', 'translate','content' , 'android', 'java', 'res')
      const androidIconSource = path.join(braveAppDir, 'theme', 'brave', 'android', androidIconSet)
      const androidIconDest = path.join(config.srcDir, 'chrome', 'android', 'java', 'res_chromium')
      const androidIconBaseSource = path.join(braveAppDir, 'theme', 'brave', 'android', androidIconSet + '_base')
      const androidIconBaseDest = path.join(config.srcDir, 'chrome', 'android', 'java', 'res_chromium_base')
      const androidResSource = path.join(config.braveCoreDir, 'android', 'java', 'res')
      const androidResDest = path.join(config.srcDir, 'chrome', 'android', 'java', 'res')
      const androidResTemplateSource = path.join(config.braveCoreDir, 'android', 'java', 'res_template')
      const androidResTemplateDest = path.join(config.srcDir, 'chrome', 'android', 'java', 'res_template')
      const androidContentPublicResSource = path.join(config.braveCoreDir, 'content', 'public', 'android', 'java', 'res')
      const androidContentPublicResDest = path.join(config.srcDir, 'content', 'public', 'android', 'java', 'res')
      const androidTouchtoFillResSource = path.join(config.braveCoreDir, 'browser', 'touch_to_fill', 'android', 'internal', 'java', 'res')
      const androidTouchtoFillResDest = path.join(config.srcDir, 'chrome', 'browser', 'touch_to_fill', 'android', 'internal', 'java', 'res')
      const androidToolbarResSource = path.join(config.braveCoreDir, 'browser', 'ui', 'android', 'toolbar', 'java', 'res')
      const androidToolbarResDest = path.join(config.srcDir, 'chrome', 'browser', 'ui', 'android', 'toolbar', 'java', 'res')
      const androidComponentsWidgetResSource = path.join(config.braveCoreDir, 'components', 'browser_ui', 'widget', 'android', 'java', 'res')
      const androidComponentsWidgetResDest = path.join(config.srcDir, 'components', 'browser_ui', 'widget', 'android', 'java', 'res')
      const androidComponentsStylesResSource = path.join(config.braveCoreDir, 'components', 'browser_ui', 'styles', 'android', 'java', 'res')
      const androidComponentsStylesResDest = path.join(config.srcDir, 'components', 'browser_ui', 'styles', 'android', 'java', 'res')
      const androidSafeBrowsingResSource = path.join(config.braveCoreDir, 'browser', 'safe_browsing', 'android', 'java', 'res')
      const androidSafeBrowsingResDest = path.join(config.srcDir, 'chrome', 'browser', 'safe_browsing', 'android', 'java', 'res')
      const androidDownloadInternalResSource = path.join(config.braveCoreDir, 'browser', 'download', 'internal', 'android', 'java', 'res')
      const androidDownloadInternalResDest = path.join(config.srcDir, 'chrome', 'browser', 'download', 'internal', 'android', 'java', 'res')
      const androidFeaturesTabUiResSource = path.join(config.braveCoreDir, 'android', 'features', 'tab_ui', 'java', 'res')
      const androidFeaturesTabUiDest = path.join(config.srcDir, 'chrome', 'android', 'features', 'tab_ui', 'java', 'res')
      const androidComponentsOmniboxResSource = path.join(config.braveCoreDir, 'components', 'omnibox', 'browser', 'android', 'java', 'res')
      const androidComponentsOmniboxResDest = path.join(config.srcDir, 'components', 'omnibox', 'browser', 'android', 'java', 'res')
      const androidBrowserUiOmniboxResSource = path.join(config.braveCoreDir, 'browser', 'ui', 'android', 'omnibox', 'java', 'brave_res')
      const androidBrowserUiOmniboxResDest = path.join(config.srcDir, 'chrome', 'browser', 'ui', 'android', 'omnibox', 'java', 'res')
      const androidBrowserPrivateResSource = path.join(config.braveCoreDir, 'browser', 'incognito', 'android', 'java', 'res')
      const androidBrowserPrivateResDest = path.join(config.srcDir, 'chrome', 'browser', 'incognito', 'android', 'java', 'res')

      // Mapping for copying Brave's Android resource into chromium folder.
      const copyAndroidResourceMapping = {
        [androidTranslateResSource]: [androidTranslateResDest],
        [androidIconSource]: [androidIconDest],
        [androidIconBaseSource]: [androidIconBaseDest],
        [androidResSource]: [androidResDest],
        [androidResTemplateSource]: [androidResTemplateDest],
        [androidContentPublicResSource]: [androidContentPublicResDest],
        [androidTouchtoFillResSource]: [androidTouchtoFillResDest],
        [androidToolbarResSource]: [androidToolbarResDest],
        [androidComponentsWidgetResSource]: [androidComponentsWidgetResDest],
        [androidComponentsStylesResSource]: [androidComponentsStylesResDest],
        [androidSafeBrowsingResSource]: [androidSafeBrowsingResDest],
        [androidDownloadInternalResSource]: [androidDownloadInternalResDest],
        [androidFeaturesTabUiResSource]: [androidFeaturesTabUiDest],
        [androidComponentsOmniboxResSource]: [androidComponentsOmniboxResDest],
        [androidBrowserUiOmniboxResSource]: [androidBrowserUiOmniboxResDest],
        [androidBrowserPrivateResSource]: [androidBrowserPrivateResDest]
      }

      console.log('copy Android app icons and app resources')
      Object.entries(copyAndroidResourceMapping).map(([sourcePath, destPaths]) => {
        let androidSourceFiles = []
        if (fs.statSync(sourcePath).isDirectory()) {
          androidSourceFiles = util.walkSync(sourcePath)
        } else {
          androidSourceFiles = [sourcePath]
        }

        for (const destPath of destPaths) {
          for (const androidSourceFile of androidSourceFiles) {
            let destinationFile = path.join(destPath, path.relative(sourcePath, androidSourceFile))
            if (!fs.existsSync(destinationFile) || util.calculateFileChecksum(androidSourceFile) != util.calculateFileChecksum(destinationFile)) {
              fs.copySync(androidSourceFile, destinationFile)
            }
            braveOverwrittenFiles.add(destinationFile);
          }
        }
      })
      removeUnlistedAndroidResources(braveOverwrittenFiles)
    }
    Log.progressFinish('update branding')
  },

  touchOverriddenChromiumSrcFiles: () => {
    Log.progressStart('touch original files overridden by chromium_src')

    // Return true when original file of |file| should be touched.
    const applyFileFilter = (file) => {
      // Only include overridable files.
      const supportedExts = ['.cc', '.h', '.json', '.mm', '.mojom', '.py', '.pdl'];
      return supportedExts.includes(path.extname(file))
    }

    const chromiumSrcDir = path.join(config.srcDir, 'brave', 'chromium_src')
    var sourceFiles = util.walkSync(chromiumSrcDir, applyFileFilter)
    const additionalGen = getAdditionalGenLocation()

    // Touch original files by updating mtime.
    let isDirty = false
    const chromiumSrcDirLen = chromiumSrcDir.length
    sourceFiles.forEach(chromiumSrcFile => {
      const relativeChromiumSrcFile = chromiumSrcFile.slice(chromiumSrcDirLen)
      let overriddenFile = path.join(config.srcDir, relativeChromiumSrcFile)
      if (fs.existsSync(overriddenFile)) {
        // If overriddenFile is older than file in chromium_src, touch it to trigger rebuild.
        isDirty |= updateFileUTimesIfOverrideIsNewer(overriddenFile, chromiumSrcFile)
      } else {
        // If the original file doesn't exist, assume that it's in the gen dir.
        overriddenFile = path.join(config.outputDir, 'gen', relativeChromiumSrcFile)
        isDirty |= deleteFileIfOverrideIsNewer(overriddenFile, chromiumSrcFile)
        // Also check the secondary gen dir, if exists
        if (!!additionalGen) {
          overriddenFile = path.join(config.outputDir, additionalGen, 'gen', relativeChromiumSrcFile)
          isDirty |= deleteFileIfOverrideIsNewer(overriddenFile, chromiumSrcFile)
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

  touchOverriddenVectorIconFiles: () => {
    Log.progressStart('touch original vector icon files overridden by brave/vector_icons')

    // Return true when original file of |file| should be touched.
    const applyFileFilter = (file) => {
      // Only includes icon files.
      const ext = path.extname(file)
      if (ext !== '.icon') { return false }
      return true
    }

    const braveVectorIconsDir = path.join(config.srcDir, 'brave', 'vector_icons')
    var braveVectorIconFiles = util.walkSync(braveVectorIconsDir, applyFileFilter)

    // Touch original files by updating mtime.
    const braveVectorIconsDirLen = braveVectorIconsDir.length
    braveVectorIconFiles.forEach(braveVectorIconFile => {
      var overriddenFile = path.join(config.srcDir, braveVectorIconFile.slice(braveVectorIconsDirLen))
      if (fs.existsSync(overriddenFile)) {
        // If overriddenFile is older than file in vector_icons, touch it to trigger rebuild.
        updateFileUTimesIfOverrideIsNewer(overriddenFile, braveVectorIconFile)
      }
    })
    Log.progressFinish('touch original vector icon files overridden by brave/vector_icons')
  },

  touchOverriddenFiles: () => {
    Log.progressScope('touch overridden files', () => {
      util.touchOverriddenChromiumSrcFiles()
      util.touchOverriddenVectorIconFiles()
    })
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
        config.srcDir, 'third_party', 'catapult', 'third_party', 'gsutil',
        'third_party', 'mock', 'ChangeLog')
    if (!fs.existsSync(changeLogFile)) {
      fs.writeFileSync(changeLogFile, '')
    }
  },

  // Chromium compares pre-installed midl files and generated midl files from IDL during the build to check integrity.
  // Generated files during the build time and upstream pre-installed files are different because we use different IDL file.
  // So, we should copy our pre-installed files to overwrite upstream pre-installed files.
  // After checking, pre-installed files are copied to gen dir and they are used to compile.
  // So, this copying in every build doesn't affect compile performance.
  updateMidlFiles: () => {
    Log.progressScope('update midl files', () => {
      const files = fs.readdirSync(path.join(
          config.braveCoreDir,
          'win_build_output', 'midl'))
      for (const file of files) {
        const srcFile = path.join(config.braveCoreDir,
            'win_build_output',
            'midl', file)
        const dstFile = path.join(config.srcDir,
            'third_party',
            'win_build_output', 'midl', file)
        try {
          const stat = fs.lstatSync(srcFile);
          // only copy the directories here
          // they each have a structure with x86/x64/arm64 versions of the files
          if (stat.isDirectory()) {
            fs.copySync(srcFile, dstFile)
          }
        } catch (e) {
          throw new Error('error copying file \"' +
              srcFile + "\"  to \"" +
              dstFile + "\"", {
                cause: e
              })
        }
      }
    })
  },

  buildNativeRedirectCC: (options = config.defaultOptions) => {
    // Expected path to redirect_cc.
    const redirectCC = path.join(config.nativeRedirectCCDir, util.appendExeIfWin32('redirect_cc'))

    // Only build if the source has changed.
    if (fs.existsSync(redirectCC) &&
        fs.statSync(redirectCC).mtime >=
        fs.statSync(path.join(config.braveCoreDir, 'tools', 'redirect_cc', 'redirect_cc.cc')).mtime) {
      return
    }

    Log.progressStart('build redirect_cc')
    gnArgs = {
      'import("//brave/tools/redirect_cc/args.gni")': null,
      use_remoteexec: config.useRemoteExec,
      rbe_exec_root: config.rbeExecRoot,
      rbe_bin_dir: config.realRewrapperDir,
      real_rewrapper: path.join(config.realRewrapperDir, 'rewrapper'),
    }

    const buildArgsStr = util.buildArgsToString(gnArgs)
    util.run('gn', ['gen', config.nativeRedirectCCDir, '--args="' + buildArgsStr + '"'], options)

    util.buildTarget('brave/tools/redirect_cc', mergeWithDefault({outputDir: config.nativeRedirectCCDir}))
    Log.progressFinish('build redirect_cc')
  },

  runGnGen: (options) => {
    const buildArgsStr = util.buildArgsToString(config.buildArgs())
    const buildArgsFile = path.join(config.outputDir, 'brave_build_args.txt')
    const buildNinjaFile = path.join(config.outputDir, 'build.ninja')
    const gnArgsFile = path.join(config.outputDir, 'args.gn')
    const prevBuildArgs = fs.existsSync(buildArgsFile) ?
      fs.readFileSync(buildArgsFile) : undefined
    const extraGnGenOptsFile = path.join(config.outputDir, 'brave_extra_gn_gen_opts.txt')
    const prevExtraGnGenOpts = fs.existsSync(extraGnGenOptsFile) ?
      fs.readFileSync(extraGnGenOptsFile) : undefined

    const shouldRunGnGen = config.force_gn_gen ||
      !fs.existsSync(buildNinjaFile) || !fs.existsSync(gnArgsFile) ||
      !prevBuildArgs || prevBuildArgs != buildArgsStr ||
      !prevExtraGnGenOpts || prevExtraGnGenOpts != config.extraGnGenOpts

    if (shouldRunGnGen) {
      // `gn gen` can modify args.gn even if it's failed.
      // Therefore delete the file to make sure that args.gn and
      // brave_build_args.txt are in sync.
      if (prevBuildArgs)
        fs.removeSync(buildArgsFile)

      util.run('gn', ['gen', config.outputDir, '--args="' + buildArgsStr + '"', config.extraGnGenOpts], options)
      fs.writeFileSync(buildArgsFile, buildArgsStr)
      fs.writeFileSync(extraGnGenOptsFile, config.extraGnGenOpts)
    }
  },

  generateNinjaFiles: (options = config.defaultOptions) => {
    Log.progressScope('generate ninja files', () => {
      util.buildNativeRedirectCC()

      if (config.getTargetOS() === 'win') {
        util.updateMidlFiles()
      }
      util.runGnGen(options)
    })
  },

  buildTarget: (target = config.buildTarget, options = config.defaultOptions) => {
    const buildId = crypto.randomUUID()
    const progressMessage = 'build ' + target + ' (id=' + buildId + ')'
    Log.progressStart(progressMessage)

    let num_compile_failure = 1
    if (config.ignore_compile_failure)
      num_compile_failure = 0

    let ninjaOpts = [
      '-C', options.outputDir || config.outputDir, target,
      '-k', num_compile_failure,
      ...config.extraNinjaOpts
    ]

    // Setting `AUTONINJA_BUILD_ID` allows tracing remote execution which helps
    // with debugging issues (e.g., slowness or remote-failures).
    options.env.AUTONINJA_BUILD_ID = buildId
    Log.progressScope(`ninja ${target} ${config.buildConfig}`, () => {
      util.run('autoninja', ninjaOpts, options)
    })

    Log.progressFinish(progressMessage)
  },

  generateXcodeWorkspace: (options = config.defaultOptions) => {
    console.log('generating Xcode workspace for "' + config.xcode_gen_target + '"...')

    const args = util.buildArgsToString(config.buildArgs())
    const genScript = path.join(config.braveCoreDir, 'vendor', 'gn-project-generators', 'xcode.py')

    const genArgs = [
      'gen', config.outputDir + "_Xcode",
      '--args="' + args + '"',
      '--ide=json',
      '--json-ide-script="' + genScript + '"',
      '--filters="' + config.xcode_gen_target + '"'
    ]

    util.run('gn', genArgs, options)
  },

  presubmit: (options = {}) => {
    if (!options.base) {
      options.base = 'origin/master'
    }
    // Temporary cleanup call, should be removed when everyone will remove
    // 'gerrit.host' from their brave checkout.
    util.runGit(
        config.braveCoreDir, ['config', '--unset-all', 'gerrit.host'], true)
    let cmd_options = config.defaultOptions
    cmd_options.cwd = config.braveCoreDir
    cmd_options = mergeWithDefault(cmd_options)
    cmd = 'git'
    // --upload mode is similar to `git cl upload`. Non-upload mode covers less
    // checks.
    args = ['cl', 'presubmit', options.base, '--force', '--upload']
    if (options.all)
      args.push('--all')
    if (options.files)
      args.push('--files', `"${options.files}"`)
    if (options.verbose) {
      args.push(...Array(options.verbose).fill('--verbose'))
    }
    if (options.fix) {
      cmd_options.env.PRESUBMIT_FIX = '1'
    }
    util.run(cmd, args, cmd_options)
  },

  format: (options = {}) => {
    if (!options.base) {
      options.base = 'origin/master'
    }
    let cmd_options = config.defaultOptions
    cmd_options.cwd = config.braveCoreDir
    cmd_options = mergeWithDefault(cmd_options)
    cmd = 'git'
    args = ['cl', 'format', '--upstream=' + options.base]
    if (options.full)
      args.push('--full')

    // Keep in sync with CheckPatchFormatted presubmit check.
    args.push('--python')
    args.push('--no-rust-fmt')

    util.run(cmd, args, cmd_options)
  },

  massRename: (options = {}) => {
    let cmd_options = config.defaultOptions
    cmd_options.cwd = config.braveCoreDir
    util.run('python3', [path.join(config.srcDir, 'tools', 'git', 'mass-rename.py')], cmd_options)
  },

  runGClient: (args, options = {}, gClientFile = config.gClientFile) => {
    if (config.gClientVerbose) {
      args.push('--verbose')
    }
    options.cwd = options.cwd || config.rootDir
    options = mergeWithDefault(options)
    options.env.GCLIENT_FILE = gClientFile
    util.run('gclient', args, options)
  },

  applyPatches: () => {
    return applyPatches()
  },

  buildArgsToString: (buildArgs) => {
    let args = ''
    for (let arg in buildArgs) {
      let val = buildArgs[arg]
      if (val !== null) {
        if (typeof val === 'string') {
          val = '"' + val + '"'
        } else {
          val = JSON.stringify(val)
        }
      }
      args += val ? arg + '=' + val + ' ' : arg + ' '
    }
    // "D:\" should be escaped as \"D:\\\\\".
    args = args.replace(/(\\+)(")/g,'$1$1$1$1$2')
    return args.replace(/"/g,'\\"')
  },

  walkSync: (dir, filter = null, filelist = []) => {
    fs.readdirSync(dir).forEach(file => {
      if (fs.statSync(path.join(dir, file)).isDirectory()) {
        filelist = util.walkSync(path.join(dir, file), filter, filelist)
      } else if (!filter || filter.call(null, file)) {
        filelist = filelist.concat(path.join(dir, file))
      }
    })
    return filelist
  },

  appendExeIfWin32: (input) => {
    if (process.platform === 'win32')
      input += '.exe'
    return input
  },

  readJSON: (file, default_value = undefined) => {
    if (!fs.existsSync(file)) {
      return default_value
    }
    return fs.readJSONSync(file)
  },

  writeJSON: (file, value) => {
    return fs.writeJSONSync(file, value, {spaces: 2})
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
}

module.exports = util
