/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { OpenSettingsPage } from './open_settings_page.js'

// One action in a gesture's chain. Only one param is meaningful at a
// time, depending on `command`.
export interface ActionRow {
  command: number
  textParam: string
  openSettingsPage: OpenSettingsPage
}

export interface DisallowedSiteRow {
  origin: string
}

// A mouse rocker action row (rock left/right); unlike other action rows,
// each is its own standalone list pref, named by `prefName`.
export interface MouseRockerActionRow {
  prefName: string
  actionRows: ActionRow[]
}

// A mouse wheel action row (wheel up/down); see `MouseRockerActionRow`.
export interface MouseWheelActionRow {
  prefName: string
  actionRows: ActionRow[]
}
