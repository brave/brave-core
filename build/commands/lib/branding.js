// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const config = require('./config')
const fs = require('fs-extra')
const l10nUtil = require('./l10nUtil')
const util = require('./util')
const Log = require('./logging')

exports.update = () => {
  Log.progressStart('update branding')
  const braveAppDir = path.join(config.braveCoreDir, 'app')
  const braveChromiumResources = path.join(
    config.braveCoreDir,
    'build',
    'chromium',
    'resources',
  )

  let fileMap = new Set()
  fileMap.add([path.join(braveChromiumResources), path.join(config.srcDir)])

  // Replace webui CSS to use our fonts.
  fileMap.add([
    path.join(
      config.braveCoreDir,
      'ui',
      'webui',
      'resources',
      'css',
      'text_defaults_md.css',
    ),
    path.join(
      config.srcDir,
      'ui',
      'webui',
      'resources',
      'css',
      'text_defaults_md.css',
    ),
  ])

  // Replace chrome dark logo with channel specific brave logo.
  fileMap.add([
    path.join(
      config.braveCoreDir,
      'node_modules',
      '@brave',
      'leo',
      'icons',
      config.getBraveLogoIconName(),
    ),
    path.join(
      config.srcDir,
      'ui',
      'webui',
      'resources',
      'images',
      'chrome_logo_dark.svg',
    ),
  ])

  // Replace webui bookmark svg icon.
  fileMap.add([
    path.join(
      config.braveCoreDir,
      'node_modules',
      '@brave',
      'leo',
      'icons',
      'browser-bookmark-normal.svg',
    ),
    path.join(
      config.srcDir,
      'ui',
      'webui',
      'resources',
      'images',
      'icon_bookmark.svg',
    ),
  ])

  for (const [source, output] of fileMap) {
    if (!fs.existsSync(source)) {
      console.warn(
        `Warning: The following file-system entry was not found for copying contents to a chromium destination: ${source}. Consider removing the entry from the file-map, or investigating whether the correct source code reference is checked out.`,
      )
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
      if (path.basename(sourceFile) === 'README.md') {
        continue
      }
      const destinationFile = path.join(
        output,
        path.relative(source, sourceFile),
      )
      if (
        !fs.existsSync(destinationFile)
        || util.calculateFileChecksum(sourceFile)
          !== util.calculateFileChecksum(destinationFile)
      ) {
        fs.copySync(sourceFile, destinationFile)
        console.log(sourceFile + ' copied to ' + destinationFile)
      }
    }
  }

  if (config.targetOS === 'android') {
    let braveOverwrittenFiles = new Set()
    const removeUnlistedAndroidResources = (braveOverwrittenFiles) => {
      const suspectedDir = path.join(
        config.srcDir,
        'chrome',
        'android',
        'java',
        'res',
      )

      let untrackedChromiumFiles = util
        .runGit(
          suspectedDir,
          ['ls-files', '--others', '--exclude-standard'],
          true,
        )
        .split('\n')
      let untrackedChromiumPaths = []
      for (const untrackedChromiumFile of untrackedChromiumFiles) {
        untrackedChromiumPath = path.join(suspectedDir, untrackedChromiumFile)

        if (!fs.statSync(untrackedChromiumPath).isDirectory()) {
          untrackedChromiumPaths.push(untrackedChromiumPath)
        }
      }

      const isChildOf = (child, parent) => {
        const relative = path.relative(parent, child)
        return (
          relative && !relative.startsWith('..') && !path.isAbsolute(relative)
        )
      }

      for (const untrackedChromiumPath of untrackedChromiumPaths) {
        if (
          isChildOf(untrackedChromiumPath, suspectedDir)
          && !braveOverwrittenFiles.has(untrackedChromiumPath)
        ) {
          fs.removeSync(untrackedChromiumPath)
          console.log(`Deleted not listed file: ${untrackedChromiumPath}`)
        }
      }
    }

    let androidIconSet = ''
    if (config.channel === 'development') {
      androidIconSet = 'res_brave_default'
    } else if (config.channel === '') {
      androidIconSet = 'res_brave'
    } else if (config.channel === 'beta') {
      androidIconSet = 'res_brave_beta'
    } else if (config.channel === 'dev') {
      androidIconSet = 'res_brave_dev'
    } else if (config.channel === 'nightly') {
      androidIconSet = 'res_brave_nightly'
    }

    const androidResSource = path.join(
      config.braveCoreDir,
      'android',
      'java',
      'res',
    )
    const androidResDest = path.join(
      config.srcDir,
      'chrome',
      'android',
      'java',
      'res',
    )
    const androidFeaturesTabUiResSource = path.join(
      config.braveCoreDir,
      'android',
      'features',
      'tab_ui',
      'java',
      'res',
    )
    const androidFeaturesTabUiDest = path.join(
      config.srcDir,
      'chrome',
      'android',
      'features',
      'tab_ui',
      'java',
      'res',
    )
    const androidDownloadInternalResSource = path.join(
      config.braveCoreDir,
      'browser',
      'download',
      'internal',
      'android',
      'java',
      'res',
    )
    const androidDownloadInternalResDest = path.join(
      config.srcDir,
      'chrome',
      'browser',
      'download',
      'internal',
      'android',
      'java',
      'res',
    )
    const androidIconSource = path.join(
      braveAppDir,
      'theme',
      'brave',
      'android',
      androidIconSet,
    )
    const androidIconDest = path.join(
      config.srcDir,
      'chrome',
      'android',
      'java',
      'res_chromium',
    )
    const androidIconBaseSource = path.join(
      braveAppDir,
      'theme',
      'brave',
      'android',
      androidIconSet + '_base',
    )
    const androidIconBaseDest = path.join(
      config.srcDir,
      'chrome',
      'android',
      'java',
      'res_chromium_base',
    )

    // Mapping for copying Brave's Android resource into chromium folder.
    const copyAndroidResourceMapping = {
      [androidResSource]: [androidResDest],
      [androidFeaturesTabUiResSource]: [androidFeaturesTabUiDest],
      [androidDownloadInternalResSource]: [androidDownloadInternalResDest],
      [androidIconSource]: [androidIconDest],
      [androidIconBaseSource]: [androidIconBaseDest],
    }

    console.log('copy Android app icons and app resources')
    Object.entries(copyAndroidResourceMapping).map(
      ([sourcePath, destPaths]) => {
        let androidSourceFiles = []
        if (fs.statSync(sourcePath).isDirectory()) {
          androidSourceFiles = util.walkSync(sourcePath)
        } else {
          androidSourceFiles = [sourcePath]
        }

        for (const destPath of destPaths) {
          for (const androidSourceFile of androidSourceFiles) {
            let destinationFile = path.join(
              destPath,
              path.relative(sourcePath, androidSourceFile),
            )
            if (
              !fs.existsSync(destinationFile)
              || util.calculateFileChecksum(androidSourceFile)
                !== util.calculateFileChecksum(destinationFile)
            ) {
              fs.copySync(androidSourceFile, destinationFile)
            }
            braveOverwrittenFiles.add(destinationFile)
          }
        }
      },
    )
    removeUnlistedAndroidResources(braveOverwrittenFiles)
  }
  Log.progressFinish('update branding')
}
