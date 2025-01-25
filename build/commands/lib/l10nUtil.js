/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * This file manages the following:
 * - Lists of files needed to be translated (Which is all top level GRD and JSON files)
 * - All mappings for auto-generated Brave files from the associated Chromium files.
 * - Top level global string replacements, such as replacing Chromium with Brave
 */

const path = require('path')
const fs = require('fs')
const chalk = require('chalk')
const { JSDOM } = require("jsdom")
const config = require('./config')

// Change to `true` for verbose console log output of GRD traversal
const verboseLogFindGrd = false
const srcDir = config.srcDir

// chromium_strings.grd and any of its parts files that we track localization for in transifex
// These map to brave/app/resources/chromium_strings*.xtb
const chromiumStringsPath = path.resolve(path.join(srcDir, 'chrome', 'app', 'chromium_strings.grd'))
const braveStringsPath = path.resolve(path.join(srcDir, 'brave', 'app', 'brave_strings.grd'))
const chromiumSettingsPartPath = path.resolve(path.join(srcDir, 'chrome', 'app', 'settings_chromium_strings.grdp'))
const braveSettingsPartPath = path.resolve(path.join(srcDir, 'brave', 'app', 'settings_brave_strings.grdp'))

//Replace android strings.
const androidChromeStringsPath = path.resolve(path.join(srcDir, 'chrome', 'browser', 'ui', 'android', 'strings', 'android_chrome_strings.grd'))
const braveAndroidChromeStringsPath = path.resolve(path.join(srcDir, 'brave', 'browser', 'ui', 'android', 'strings', 'android_chrome_strings.grd'))
const androidTabUiStringsPath = path.resolve(path.join(srcDir, 'chrome', 'android', 'features', 'tab_ui', 'java', 'strings', 'android_chrome_tab_ui_strings.grd'))
const braveAndroidTabUiStringsPath = path.resolve(path.join(srcDir, 'brave', 'android', 'features', 'tab_ui', 'java', 'strings', 'android_chrome_tab_ui_strings.grd'))
const androidWebappsStringsPath = path.resolve(path.join(srcDir, 'components', 'webapps', 'browser', 'android', 'android_webapps_strings.grd'))
const braveAndroidWebappsStringsPath = path.resolve(path.join(srcDir, 'brave', 'components', 'webapps', 'browser', 'android', 'android_webapps_strings.grd'))
const androidBrowserUiStringsPath = path.resolve(path.join(srcDir, 'components', 'browser_ui', 'strings', 'android', 'browser_ui_strings.grd'))
const braveAndroidBrowserUiStringsPath = path.resolve(path.join(srcDir, 'brave', 'components', 'browser_ui', 'strings', 'android', 'browser_ui_strings.grd'))


// component_chromium_strings.grd and any of its parts files that we track localization for in transifex
// These map to brave/app/strings/components_chromium_strings*.xtb
const chromiumComponentsChromiumStringsPath = path.resolve(path.join(srcDir, 'components', 'components_chromium_strings.grd'))
const braveComponentsBraveStringsPath = path.resolve(path.join(srcDir, 'brave', 'components', 'components_brave_strings.grd'))

// components/component_strings.grd and any of its parts files that we track localization for in transifex
// These map to brave/components/component_strings*.xtb
const chromiumComponentsStringsPath = path.resolve(path.join(srcDir, 'components', 'components_strings.grd'))
const braveComponentsStringsPath = path.resolve(path.join(srcDir, 'brave', 'components', 'components_strings.grd'))

// generated_resources.grd and any of its parts files that we track localization for in transifex
// There is also chromeos_strings.grdp, but we don't need to track it here because it is explicitly skipped in transifex.py
// These map to brave/app/resources/generated_resoruces*.xtb
const chromiumGeneratedResourcesPath = path.resolve(path.join(srcDir, 'chrome', 'app', 'generated_resources.grd'))
const braveGeneratedResourcesPath = path.resolve(path.join(srcDir, 'brave', 'app', 'generated_resources.grd'))
const chromiumGeneratedResourcesExcludes = new Set(["chromeos_strings.grdp"])

// The following are not generated files but still need to be tracked so they get sent to transifex
// These xtb files don't need to be copied anywhere.
// brave_generated_resources.grd maps to brave/app/resources/brave_generated_resources*.xtb,
// brave_components_strings.grd maps to brave/components/resources/strings/brave_components_resources*.xtb
// messages.json localization is handled inside of brave-extension.
const braveSpecificGeneratedResourcesPath = path.resolve(path.join(srcDir, 'brave', 'app', 'brave_generated_resources.grd'))
const braveResourcesComponentsStringsPath = path.resolve(path.join(srcDir, 'brave', 'components', 'resources', 'brave_components_strings.grd'))
const braveExtensionMessagesPath = path.resolve(path.join(srcDir, 'brave', 'components', 'brave_extension', 'extension', 'brave_extension', '_locales', 'en_US', 'messages.json'))
const braveAndroidBraveStringsPath = path.resolve(path.join(srcDir, 'brave', 'browser', 'ui', 'android', 'strings', 'android_brave_strings.grd'))
const braveAndroidPlaylistStringsPath = path.resolve(path.join(srcDir, 'brave','browser','playlist', 'android', 'java', 'org', 'chromium','chrome','browser','playlist', 'android_playlist_strings.grd'))

// Helper function to find all grdp parts in a grd.
function getGrdPartsFromGrd(path) {
  const grd = new JSDOM(fs.readFileSync(path, 'utf8'))
  const partTags = grd.window.document.getElementsByTagName("part")
  let parts = new Array()
  for (const partTag of partTags) {
    parts.push(partTag.getAttribute('file'));
  }
  return parts
}

// Helper function to create a mapping for grd and all of its grdp parts.
function addGrd(chromiumPath, bravePath, exclude = new Set()) {
  if (verboseLogFindGrd)
    console.log("Adding mappings for GRD: " + chromiumPath)
  if (!fs.existsSync(chromiumPath)) {
    const err = new Error(`addGrd: Error. File not found at path "${chromiumPath}"`)
    console.error(err)
    throw err
  }
  let mapping = {}
  // Add grd parts before grd because chromium-rebase-l10n.py expects them to be
  // processed first.
  const grdps = getGrdPartsFromGrd(chromiumPath)
  if (grdps.length) {
    const chromiumDir = path.dirname(chromiumPath)
    const braveDir = path.dirname(bravePath)
    for (const grdp of grdps) {
      if (exclude.has(grdp)) {
        continue
      }
      const chromiumGrdpPath = path.resolve(path.join(chromiumDir, grdp))
      const braveGrdpPath = path.resolve(path.join(braveDir, grdp))
      // grdp files can have their own grdp parts too
      mapping = { ...mapping, ...addGrd(chromiumGrdpPath, braveGrdpPath, exclude) }
    }
    if (verboseLogFindGrd)
      console.log("  - Added " + (Object.keys(mapping).length - 1) + " GRDP.")
  }
  mapping[chromiumPath] = bravePath
  return mapping
}

// Helper functions that's, for a given pair of chromium to brave GRD mapping
// from the supplied map, determines which GRDP parts are no longer present in
// the chromium GRD file.
function getRemovedGRDParts(mapping) {
  let removedMap = new Map()
  for (const [sourcePath, destPath] of Object.entries(mapping)) {
    if (path.extname(destPath) === ".grd") {
      const braveGRDPs = getGrdPartsFromGrd(destPath)
      const chromiumGRDPs = getGrdPartsFromGrd(sourcePath)
      let removed = new Set()
      for (let i = 0; i < braveGRDPs.length; i++) {
        if (!chromiumGRDPs.includes(braveGRDPs[i])) {
          removed.add(braveGRDPs[i])
        }
      }
      if (removed.size) {
        removedMap.set(destPath, removed)
      }
    }
  }
  return removedMap
}

// Add all GRD mappings here.
function getAutoGeneratedGrdMappings() {
  if (typeof(getAutoGeneratedGrdMappings.mappings) === 'undefined') {
    console.log(chalk.italic('Recursing through GRD to find GRDP files...'))
    // Brave specific only grd and grdp files should NOT be added.
    // Using AddGrd will add GRD and all of its GRDPs.
    getAutoGeneratedGrdMappings.mappings = {
      ...addGrd(chromiumComponentsStringsPath, braveComponentsStringsPath),
      ...addGrd(chromiumGeneratedResourcesPath, braveGeneratedResourcesPath, chromiumGeneratedResourcesExcludes),
      ...addGrd(androidChromeStringsPath, braveAndroidChromeStringsPath),
      ...addGrd(androidTabUiStringsPath, braveAndroidTabUiStringsPath),
      ...addGrd(androidWebappsStringsPath, braveAndroidWebappsStringsPath),
      ...addGrd(androidBrowserUiStringsPath, braveAndroidBrowserUiStringsPath)
    }
    console.log(chalk.italic('Done recursing through GRD to find GRDP files.'))
  }
  return getAutoGeneratedGrdMappings.mappings
}

function getChromiumToAutoGeneratedBraveMapping() {
  if (typeof(getChromiumToAutoGeneratedBraveMapping.mapping) === 'undefined') {
    // When adding new grd or grdp files, never add a grdp part path without a
    // parent grd path, but add the grd parts to the mapping before the parent
    // grd, becase chromium-rebase-l10n.py expects them to be processed first.
    // Group them with a leading and trailing newline to keep this file organized.
    // The first 3 are added explicitly because we change the file names.
    getChromiumToAutoGeneratedBraveMapping.mapping = {
      [chromiumSettingsPartPath]: braveSettingsPartPath,
      [chromiumStringsPath]: braveStringsPath,

      [chromiumComponentsChromiumStringsPath]: braveComponentsBraveStringsPath,

      ...getAutoGeneratedGrdMappings()
    }
  }
  return getChromiumToAutoGeneratedBraveMapping.mapping
}

const l10nUtil = {
  // Same as with chromiumToAutoGeneratedBraveMapping but maps in the opposite direction
  getAutoGeneratedBraveToChromiumMapping: () => {
    if (typeof(l10nUtil.getAutoGeneratedBraveToChromiumMapping.mapping) === 'undefined') {
      const chromiumToAutoGeneratedBraveMapping = getChromiumToAutoGeneratedBraveMapping()
      l10nUtil.getAutoGeneratedBraveToChromiumMapping.mapping = Object.keys(
        chromiumToAutoGeneratedBraveMapping).reduce((obj, key) => (
          { ...obj, [chromiumToAutoGeneratedBraveMapping[key]]: key }), {})
    }
    return l10nUtil.getAutoGeneratedBraveToChromiumMapping.mapping
  },

  // All paths which are generated
  getBraveAutoGeneratedPaths: () => {
    return Object.values(getChromiumToAutoGeneratedBraveMapping())
  },

  // All paths which are not generated
  getBraveNonGeneratedPaths: () => {
    if (typeof(l10nUtil.getBraveNonGeneratedPaths.paths) === 'undefined') {
      l10nUtil.getBraveNonGeneratedPaths.paths = [
        braveSpecificGeneratedResourcesPath,
        braveResourcesComponentsStringsPath,
        braveExtensionMessagesPath,
        braveAndroidBraveStringsPath,
        braveAndroidPlaylistStringsPath
      ]
    }
    return l10nUtil.getBraveNonGeneratedPaths.paths
  },

  // Brave specific strings and Chromium mapped Brave strings will be here.
  // But you only need to add the Brave specific strings manually here.
  getAllBravePaths: () => {
    return l10nUtil.getBraveNonGeneratedPaths().concat(l10nUtil.getBraveAutoGeneratedPaths())
  },

  // Get all GRD and JSON paths whether they are generatd or not
  // Push and pull scripts for l10n use this.
  // Transifex manages files per grd and not per grd or grdp.
  // This is because only 1 xtb is created per grd per locale even if it has multiple grdp files.
  getBraveTopLevelPaths: () => {
    return l10nUtil.getAllBravePaths().filter((x) => ['grd', 'json'].includes(x.split('.').pop()))
  },

  // Helper function to pretty print removed GRDP file names.
  logRemovedGRDParts: (mapping) => {
    if (mapping.size) {
      console.log("\n**************************************************************************")
      console.log("The following GRDP files are no longer in the corresponding Chromium GRDs:\n")
      for (const [grd, grdps] of mapping.entries()) {
        console.log("  From " + grd + ":")
        for (const grdp of grdps) {
          console.log("    - " + grdp)
        }
      }
    }
  },

  // This simply copies the content of Chromium files to their mapped Brave
  // equivalents. Additionally, it removes GRDP files that are no longer in the
  // Chromium GRD files.
  rebaseBraveStringFilesOnChromiumL10nFiles: async () => {
    const removedMap = getRemovedGRDParts(getAutoGeneratedGrdMappings())
    const ops = Object.entries(getChromiumToAutoGeneratedBraveMapping()).map(
      async ([sourcePath, destPath]) => {
        console.log("Resetting " + path.relative(srcDir, destPath) + " <- " +
          path.relative(srcDir, sourcePath))
        return await fs.copyFile(sourcePath, destPath, (err) => {
          if (err) {
            console.log("Error: " + err)
          }
        })
      }
    )
    await Promise.all(ops)
    return removedMap
  },
}  // const l10nUtil

module.exports = l10nUtil
