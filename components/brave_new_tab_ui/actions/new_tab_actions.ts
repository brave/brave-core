// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { createAction } from 'redux-act'
import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/new_tab_types'
import { Stats } from '../api/stats'
import { PrivateTabData } from '../api/privateTabData'
import { TorTabData } from '../api/torTabData'
import { InitialData } from '../api/initialData'

export const statsUpdated = (stats: Stats) =>
  action(types.NEW_TAB_STATS_UPDATED, {
    stats
  })

export const init = createAction<void>('page init')

export type DismissBraveTalkPromptPayload = {
  isAutomatic: boolean
}
export const dismissBraveTalkPrompt = createAction<DismissBraveTalkPromptPayload>('dismiss brave talk prompt')

export const privateTabDataUpdated = (data: PrivateTabData) =>
  action(types.NEW_TAB_PRIVATE_TAB_DATA_UPDATED, data)

export const torTabDataUpdated = (data: TorTabData) =>
  action(types.NEW_TAB_TOR_TAB_DATA_UPDATED, data)

export const dismissBrandedWallpaperNotification = (isUserAction: boolean) =>
  action(types.NEW_TAB_DISMISS_BRANDED_WALLPAPER_NOTIFICATION, {
    isUserAction
  })

export const preferencesUpdated = (preferences: NewTab.Preferences) =>
  action(types.NEW_TAB_PREFERENCES_UPDATED, preferences)

export const clockWidgetUpdated = (showClockWidget: boolean,
    clockFormat: string) =>
  action(types.UPDATE_CLOCK_WIDGET, { showClockWidget, clockFormat })

export const setInitialData = (initialData: InitialData) =>
  action(types.NEW_TAB_SET_INITIAL_DATA, initialData)

export const setMostVisitedSettings = (showTopSites: boolean, customLinksEnabled: boolean) =>
  action(types.SET_MOST_VISITED_SITES, { showTopSites, customLinksEnabled })

export const topSitesStateUpdated = (newShowTopSites: boolean, newCustomLinksEnabled: boolean, customLinksNum: number) =>
  action(types.TOP_SITES_STATE_UPDATED, { newShowTopSites, newCustomLinksEnabled, customLinksNum })

export const customizeClicked = () => action(types.CUSTOMIZE_CLICKED, {})
