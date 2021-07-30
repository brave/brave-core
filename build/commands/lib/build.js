const config = require('../lib/config')
const util = require('../lib/util')
const path = require('path')
const fs = require('fs-extra')

const isOverrideNewer = (original, override) => {
  return (fs.statSync(override).mtimeMs - fs.statSync(original).mtimeMs > 0)
}

const updateFileUTimesIfOverrideIsNewer = (original, override) => {
  if (isOverrideNewer(original, override)) {
    const date = new Date()
    fs.utimesSync(original, date, date)
    console.log(original + ' is touched.')
  }
}

const deleteFileIfOverrideIsNewer = (original, override) => {
  if (fs.existsSync(original) && isOverrideNewer(original, override)) {
    try {
      fs.unlinkSync(original)
      console.log(original + ' has been deleted.')
    } catch(err) {
      console.error('Unable to delete file: ' + original + ' error: ', err)
      process.exit(1)
    }
  }
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

const touchOverriddenFiles = () => {
  console.log('touch original files overridden by chromium_src...')

  // Return true when original file of |file| should be touched.
  const applyFileFilter = (file) => {
    // Exclude test files
    if (file.indexOf('browsertest') > -1 || file.indexOf('unittest') > -1) { return false }

    // Only include overridable files.
    const ext = path.extname(file)
    if (ext !== '.cc' && ext !== '.h' && ext !== '.mm' && ext !== '.mojom') { return false }

    return true
  }

  const chromiumSrcDir = path.join(config.srcDir, 'brave', 'chromium_src')
  var sourceFiles = util.walkSync(chromiumSrcDir, applyFileFilter)
  const additionalGen = getAdditionalGenLocation()

  // Touch original files by updating mtime.
  const chromiumSrcDirLen = chromiumSrcDir.length
  sourceFiles.forEach(chromiumSrcFile => {
    const relativeChromiumSrcFile = chromiumSrcFile.slice(chromiumSrcDirLen)
    let overriddenFile = path.join(config.srcDir, relativeChromiumSrcFile)
    if (fs.existsSync(overriddenFile)) {
      // If overriddenFile is older than file in chromium_src, touch it to trigger rebuild.
      updateFileUTimesIfOverrideIsNewer(overriddenFile, chromiumSrcFile)
    } else {
      // If the original file doesn't exist, assume that it's in the gen dir.
      overriddenFile = path.join(config.outputDir, 'gen', relativeChromiumSrcFile)
      deleteFileIfOverrideIsNewer(overriddenFile, chromiumSrcFile)
      // Also check the secondary gen dir, if exists
      if (!!additionalGen) {
        overriddenFile = path.join(config.outputDir, additionalGen, 'gen', relativeChromiumSrcFile)
        deleteFileIfOverrideIsNewer(overriddenFile, chromiumSrcFile)
      }
    }
  })
}

const touchOverriddenVectorIconFiles = () => {
  console.log('touch original vector icon files overridden by brave/vector_icons...')

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
}

/**
 * Checks to make sure the src/chrome/VERSION matches brave-browser's package.json version
 */
const checkVersionsMatch = () => {
  const srcChromeVersionDir = path.resolve(path.join(config.srcDir, 'chrome', 'VERSION'))
  const versionData = fs.readFileSync(srcChromeVersionDir, 'utf8')
  const re = /MAJOR=(\d+)\s+MINOR=(\d+)\s+BUILD=(\d+)\s+PATCH=(\d+)/
  const found = versionData.match(re)
  const braveVersionFromChromeFile = `${found[2]}.${found[3]}.${found[4]}`
  if (braveVersionFromChromeFile !== config.braveVersion) {
    // Only a warning. The CI environment will choose to proceed or not within its own script.
    console.warn(`Version files do not match!\nsrc/chrome/VERSION: ${braveVersionFromChromeFile}\nbrave-browser package.json version: ${config.braveVersion}`)
  }
}

const build = (buildConfig = config.defaultBuildConfig, options) => {
  config.buildConfig = buildConfig
  config.update(options)
  checkVersionsMatch()

  touchOverriddenFiles()
  touchOverriddenVectorIconFiles()
  util.updateBranding()

  if (config.xcode_gen_target) {
    util.generateXcodeWorkspace()
  } else {
    util.buildTarget()
  }
}

module.exports = build
