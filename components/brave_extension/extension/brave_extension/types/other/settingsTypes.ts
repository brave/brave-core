/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export type Settings = {
  key: string
  type: any
  // TODO: can support multiple types, see PrefType in chromel.d.ts
  value: boolean
}

export type SettingsData = {
  [key in GeneratedSettingsKey]: boolean
}

export type SettingsOptions = {
  [key: string]: GeneratedSettingsKey
}

export type GeneratedSettingsKey =
  'showAdvancedView' |
  'statsBadgeVisible'

export type SettingsKey =
  'brave.shields.advanced_view_enabled' |
  'brave.shields.stats_badge_visible'
