/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  BrowserProfile,
  importDataTypes,
  ImportDataType,
} from '../api/welcome_api'

const browserNames = [
  'Google Chrome Canary',
  'Google Chrome Beta',
  'Google Chrome Dev',
  'Google Chrome',
  'Chromium',
  'Safari',
  'Mozilla Firefox',
  'Microsoft Edge',
  'Vivaldi',
  'Opera',
  'Yandex',
  'NAVER Whale',
  'Microsoft Internet Explorer',
  'Brave',
] as const

export type BrowserName = (typeof browserNames)[number]

// Splits a full profile name into browser and profile name parts.
export function splitImportProfileName(fullName: string): {
  browserName: BrowserName | null
  profileName: string
} {
  for (const browserName of browserNames) {
    if (fullName === browserName) {
      return { browserName, profileName: '' }
    }
    if (fullName.startsWith(browserName + ' ')) {
      return {
        browserName,
        profileName: fullName.slice(browserName.length + 1),
      }
    }
  }
  return { browserName: null, profileName: fullName }
}

// Returns the importable data types for the specified profile.
export function getProfileDataTypes(profile: BrowserProfile | null) {
  const dataTypes: ImportDataType[] = []
  if (profile) {
    for (const type of importDataTypes) {
      if (profile[type]) {
        dataTypes.push(type)
      }
    }
  }
  return dataTypes
}

// Returns true if the profile has importable data types.
export function profileHasImportableTypes(profile: BrowserProfile) {
  for (const type of importDataTypes) {
    if (profile[type]) {
      return true
    }
  }
  return false
}
