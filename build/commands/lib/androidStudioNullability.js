/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import fs from 'node:fs'
import path from 'node:path'

// Android Studio does not recognize Chromium's nullness annotations
// (org.chromium.build.annotations.*): they are neither JSpecify nor JSR-305
// annotations, so the IDE treats @NullMarked code as unannotated and asks for
// explicit @NonNull where NullAway needs none. Registering the annotations
// with the IDE fixes the inspections for explicitly annotated code. The
// registration is stored per-project in .idea/misc.xml, so it must be seeded
// again whenever a project is generated into a fresh output directory.
//
// Only the Chromium annotations are persisted here. When loading the
// component, the IDE unions its own built-in defaults (org.jetbrains,
// androidx, jakarta, ...) into the persisted lists, so they never need to be
// spelled out: see NullableNotNullManagerImpl.loadState, which calls
// normalizeDefaults() to add any built-in annotation missing from the lists.
const CHROMIUM_NULLABLE_ANNOTATIONS = [
  'org.chromium.build.annotations.Nullable',
  'org.chromium.build.annotations.MonotonicNonNull',
]

const CHROMIUM_NOT_NULL_ANNOTATIONS = ['org.chromium.build.annotations.NonNull']

const annotationListXml = (annotations) => {
  const items = annotations
    .map(
      (annotation, index) =>
        `          <item index="${index}" class="java.lang.String" `
        + `itemvalue="${annotation}" />`,
    )
    .join('\n')
  return `      <value>
        <list size="${annotations.length}">
${items}
        </list>
      </value>`
}

const annotationOptionXml = (name, annotations) =>
  `<option name="${name}">\n${annotationListXml(annotations)}\n    </option>`

const NULLABILITY_COMPONENT = `  <component name="NullableNotNullManager">
    <option name="myDefaultNullable" value="org.chromium.build.annotations.Nullable" />
    <option name="myDefaultNotNull" value="org.chromium.build.annotations.NonNull" />
    ${annotationOptionXml('myNullables', CHROMIUM_NULLABLE_ANNOTATIONS)}
    ${annotationOptionXml('myNotNulls', CHROMIUM_NOT_NULL_ANNOTATIONS)}
  </component>`

const EMPTY_MISC_XML = `<?xml version="1.0" encoding="UTF-8"?>
<project version="4">
</project>
`

// Merges the Chromium annotations into one annotation list option of an
// existing NullableNotNullManager component, keeping any annotations already
// listed there.
function mergeAnnotationOption(componentXml, name, chromiumAnnotations) {
  const optionMatch = componentXml.match(
    new RegExp(`<option name="${name}">[\\s\\S]*?</option>`),
  )
  if (!optionMatch) {
    return componentXml.replace(
      /\s*<\/component>/,
      `\n    ${annotationOptionXml(name, chromiumAnnotations)}\n  </component>`,
    )
  }
  const existing = [...optionMatch[0].matchAll(/itemvalue="([^"]*)"/g)].map(
    (match) => match[1],
  )
  const missing = chromiumAnnotations.filter(
    (annotation) => !existing.includes(annotation),
  )
  if (missing.length === 0) {
    return componentXml
  }
  return componentXml.replace(
    optionMatch[0],
    annotationOptionXml(name, existing.concat(missing)),
  )
}

// Seeds the NullableNotNullManager component into the Android Studio project
// at |projectDir|, creating .idea/misc.xml if needed. When the component
// already exists, the Chromium annotations are added to its lists if missing,
// keeping user customizations made through the IDE. Returns a short status
// string for logging.
function updateNullabilityAnnotations(ideaDir) {
  const miscXmlPath = path.join(ideaDir, 'misc.xml')
  if (!fs.existsSync(miscXmlPath)) {
    fs.mkdirSync(ideaDir, { recursive: true })
    fs.writeFileSync(miscXmlPath, EMPTY_MISC_XML)
  }
  const miscXml = fs.readFileSync(miscXmlPath, 'utf8')
  const componentMatch = miscXml.match(
    /<component name="NullableNotNullManager">[\s\S]*?<\/component>/,
  )
  if (!componentMatch) {
    if (!miscXml.includes('</project>')) {
      throw new Error(`Cannot update malformed ${miscXmlPath}`)
    }
    fs.writeFileSync(
      miscXmlPath,
      miscXml.replace('</project>', `${NULLABILITY_COMPONENT}\n</project>`),
    )
    return 'configured'
  }
  let merged = componentMatch[0]
  merged = mergeAnnotationOption(
    merged,
    'myNullables',
    CHROMIUM_NULLABLE_ANNOTATIONS,
  )
  merged = mergeAnnotationOption(
    merged,
    'myNotNulls',
    CHROMIUM_NOT_NULL_ANNOTATIONS,
  )
  if (merged === componentMatch[0]) {
    return 'already configured'
  }
  fs.writeFileSync(miscXmlPath, miscXml.replace(componentMatch[0], merged))
  return 'updated'
}

// The IDE cannot be taught that @NullMarked makes everything non-null by
// default, so its "annotation should be propagated to overrides" checks flag
// unannotated overrides of @NonNull members (e.g. from androidx) even though
// NullAway needs no annotation there. Disable those checks; the rest of the
// NullableProblems inspection stays on, and NullAway remains the source of
// truth at build time.
const NULLABILITY_INSPECTION_TOOL = `    <inspection_tool class="NullableProblems" enabled="true" level="WARNING" enabled_by_default="true">
      <option name="REPORT_NOT_ANNOTATED_METHOD_OVERRIDES_NOTNULL" value="false" />
      <option name="REPORT_NOT_ANNOTATED_PARAMETER_OVERRIDES_NOTNULL" value="false" />
      <option name="REPORT_ANNOTATION_NOT_PROPAGATED_TO_OVERRIDERS" value="false" />
    </inspection_tool>`

const EMPTY_INSPECTION_PROFILE_XML = `<component name="InspectionProjectProfileManager">
  <profile version="1.0">
    <option name="myName" value="Project Default" />
  </profile>
</component>
`

// Makes the project use its own inspection profile instead of an IDE-wide
// one. Only written when missing, to not override a deliberate user choice.
const PROFILE_SETTINGS_XML = `<component name="InspectionProjectProfileManager">
  <settings>
    <option name="USE_PROJECT_PROFILE" value="true" />
    <version value="1.0" />
  </settings>
</component>
`

function updateInspectionProfile(ideaDir) {
  const profileDir = path.join(ideaDir, 'inspectionProfiles')
  const profilePath = path.join(profileDir, 'Project_Default.xml')
  const settingsPath = path.join(profileDir, 'profiles_settings.xml')
  if (!fs.existsSync(profilePath)) {
    fs.mkdirSync(profileDir, { recursive: true })
    fs.writeFileSync(profilePath, EMPTY_INSPECTION_PROFILE_XML)
  }
  if (!fs.existsSync(settingsPath)) {
    fs.writeFileSync(settingsPath, PROFILE_SETTINGS_XML)
  }
  const profileXml = fs.readFileSync(profilePath, 'utf8')
  if (profileXml.includes('class="NullableProblems"')) {
    return 'already configured'
  }
  if (!profileXml.includes('</profile>')) {
    throw new Error(`Cannot update malformed ${profilePath}`)
  }
  fs.writeFileSync(
    profilePath,
    profileXml.replace(
      /\n\s*<\/profile>/,
      `\n${NULLABILITY_INSPECTION_TOOL}\n  </profile>`,
    ),
  )
  return 'configured'
}

// Configures the Android Studio project at |projectDir| to understand
// Chromium's nullness annotations and to not warn about annotations that
// @NullMarked makes redundant. Customizations made through the IDE survive:
// annotations are only ever added to existing lists, and an existing
// NullableProblems inspection entry is left untouched since its options may
// have been changed deliberately. Returns a short status string for logging.
export default function configureNullability(projectDir) {
  const ideaDir = path.join(projectDir, '.idea')
  const annotationsStatus = updateNullabilityAnnotations(ideaDir)
  const profileStatus = updateInspectionProfile(ideaDir)
  return `annotations ${annotationsStatus}, inspection profile ${profileStatus}`
}
