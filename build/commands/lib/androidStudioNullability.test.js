// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import os from 'node:os'
import fs from 'fs-extra'
import configureNullability from './androidStudioNullability.js'

const dirPrefixTmp = 'brave-browser-test-android-studio-nullability-'

const existingMiscXml = `<?xml version="1.0" encoding="UTF-8"?>
<project version="4">
  <component name="ProjectRootManager" version="2" languageLevel="JDK_21" />
</project>`

const existingProfileXml = `<component name="InspectionProjectProfileManager">
  <profile version="1.0">
    <option name="myName" value="Project Default" />
    <inspection_tool class="AndroidLintUsesMinSdkAttributes" enabled="false" level="WARNING" enabled_by_default="false" />
  </profile>
</component>`

describe('configureNullability', function () {
  let projectDir, miscXmlPath, profilePath, profileSettingsPath

  beforeEach(async function () {
    projectDir = await fs.mkdtemp(path.join(os.tmpdir(), dirPrefixTmp))
    miscXmlPath = path.join(projectDir, '.idea', 'misc.xml')
    profilePath = path.join(
      projectDir,
      '.idea',
      'inspectionProfiles',
      'Project_Default.xml',
    )
    profileSettingsPath = path.join(
      projectDir,
      '.idea',
      'inspectionProfiles',
      'profiles_settings.xml',
    )
  })

  afterEach(async function () {
    try {
      await fs.remove(projectDir)
    } catch (err) {
      console.warn(`Test cleanup: could not remove directory at ${projectDir}`)
    }
  })

  test('creates all config files when .idea does not exist', async function () {
    const status = configureNullability(projectDir)
    expect(status).toBe('annotations configured, inspection profile configured')
    const miscXml = await fs.readFile(miscXmlPath, 'utf8')
    expect(miscXml).toContain('name="NullableNotNullManager"')
    expect(miscXml).toContain('org.chromium.build.annotations.Nullable')
    expect(miscXml).toContain('org.chromium.build.annotations.MonotonicNonNull')
    expect(miscXml).toContain('org.chromium.build.annotations.NonNull')
    // Only the Chromium annotations are persisted: the IDE unions its
    // built-in defaults into the lists when loading the component.
    expect(miscXml).not.toContain('androidx.annotation.Nullable')
    expect(miscXml).not.toContain('org.jetbrains.annotations.NotNull')

    const profileXml = await fs.readFile(profilePath, 'utf8')
    expect(profileXml).toContain('class="NullableProblems"')
    expect(profileXml).toContain(
      '<option name="REPORT_NOT_ANNOTATED_PARAMETER_OVERRIDES_NOTNULL" '
        + 'value="false" />',
    )
    const settingsXml = await fs.readFile(profileSettingsPath, 'utf8')
    expect(settingsXml).toContain('name="USE_PROJECT_PROFILE" value="true"')
  })

  test('preserves existing components and inspection tools', async function () {
    await fs.outputFile(miscXmlPath, existingMiscXml)
    await fs.outputFile(profilePath, existingProfileXml)
    const status = configureNullability(projectDir)
    expect(status).toBe('annotations configured, inspection profile configured')
    const miscXml = await fs.readFile(miscXmlPath, 'utf8')
    expect(miscXml).toContain('name="ProjectRootManager"')
    expect(miscXml).toContain('name="NullableNotNullManager"')
    expect(miscXml.trim()).toMatch(/<\/project>$/)

    const profileXml = await fs.readFile(profilePath, 'utf8')
    expect(profileXml).toContain('class="AndroidLintUsesMinSdkAttributes"')
    expect(profileXml).toContain('class="NullableProblems"')
    expect(profileXml.trim()).toMatch(/<\/component>$/)
  })

  test('adds Chromium annotations to an existing component', async function () {
    const userMiscXml = `<?xml version="1.0" encoding="UTF-8"?>
<project version="4">
  <component name="NullableNotNullManager">
    <option name="myDefaultNullable" value="androidx.annotation.Nullable" />
    <option name="myNullables">
      <value>
        <list size="2">
          <item index="0" class="java.lang.String" itemvalue="androidx.annotation.Nullable" />
          <item index="1" class="java.lang.String" itemvalue="my.custom.Nullable" />
        </list>
      </value>
    </option>
  </component>
</project>`
    await fs.outputFile(miscXmlPath, userMiscXml)
    const status = configureNullability(projectDir)
    expect(status).toBe('annotations updated, inspection profile configured')
    const miscXml = await fs.readFile(miscXmlPath, 'utf8')
    // User annotations and defaults are kept, Chromium ones are appended.
    expect(miscXml).toContain(
      '<option name="myDefaultNullable" value="androidx.annotation.Nullable" />',
    )
    expect(miscXml).toContain('itemvalue="my.custom.Nullable"')
    expect(miscXml).toContain(
      'itemvalue="org.chromium.build.annotations.Nullable"',
    )
    expect(miscXml).toContain(
      'itemvalue="org.chromium.build.annotations.MonotonicNonNull"',
    )
    // The absent myNotNulls option is created with only the Chromium
    // annotations; the IDE adds its built-in defaults on load.
    expect(miscXml).toContain('<option name="myNotNulls">')
    expect(miscXml).not.toContain('itemvalue="androidx.annotation.NonNull"')
    expect(miscXml).toContain(
      'itemvalue="org.chromium.build.annotations.NonNull"',
    )
  })

  test('does not touch existing nullability configuration', async function () {
    configureNullability(projectDir)
    const miscXml = await fs.readFile(miscXmlPath, 'utf8')
    const profileXml = await fs.readFile(profilePath, 'utf8')
    const status = configureNullability(projectDir)
    expect(status).toBe(
      'annotations already configured, inspection profile already configured',
    )
    expect(await fs.readFile(miscXmlPath, 'utf8')).toBe(miscXml)
    expect(await fs.readFile(profilePath, 'utf8')).toBe(profileXml)
  })

  test('does not overwrite existing profiles_settings.xml', async function () {
    const userSettings = `<component name="InspectionProjectProfileManager">
  <settings>
    <option name="USE_PROJECT_PROFILE" value="false" />
    <version value="1.0" />
  </settings>
</component>`
    await fs.outputFile(profileSettingsPath, userSettings)
    configureNullability(projectDir)
    expect(await fs.readFile(profileSettingsPath, 'utf8')).toBe(userSettings)
  })

  test('throws on malformed misc.xml', async function () {
    await fs.outputFile(miscXmlPath, '<project version="4">')
    expect(() => configureNullability(projectDir)).toThrow('malformed')
  })
})
