// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createAction } from 'redux-act'
import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/new_tab_types'
import { Stats } from '../api/stats'
import { NewTabAdsData } from '../api/newTabAdsData'
import { InitialData } from '../api/initialData'
import { Background, CustomBackground } from '../api/background'

export const statsUpdated = (stats: Stats) =>
  action(types.NEW_TAB_STATS_UPDATED, {
    stats
  })

export const init = createAction<void>('page init')

export const newTabAdsDataUpdated = (data: NewTabAdsData) =>
  action(types.NEW_TAB_ADS_DATA_UPDATED, data)

export const dismissBrandedWallpaperNotification = (isUserAction: boolean) =>
  action(types.NEW_TAB_DISMISS_BRANDED_WALLPAPER_NOTIFICATION, {
    isUserAction
  })

export const preferencesUpdated = (preferences: NewTab.Preferences) =>
  action(types.NEW_TAB_PREFERENCES_UPDATED, preferences)

export const setInitialData = (initialData: InitialData) =>
  action(types.NEW_TAB_SET_INITIAL_DATA, initialData)

export const setMostVisitedSettings = (showTopSites: boolean, customLinksEnabled: boolean) =>
  action(types.SET_MOST_VISITED_SITES, { showTopSites, customLinksEnabled })

export const topSitesStateUpdated = (newShowTopSites: boolean, newCustomLinksEnabled: boolean, customLinksNum: number) =>
  action(types.TOP_SITES_STATE_UPDATED, { newShowTopSites, newCustomLinksEnabled, customLinksNum })

export const customizeClicked = () => action(types.CUSTOMIZE_CLICKED, {})

export const customBackgroundUpdated = (background: Background) =>
  action(types.BACKGROUND_UPDATED, { background })

export const customImageBackgroundsUpdated = (backgrounds: CustomBackground[]) =>
  action(types.CUSTOM_IMAGE_BACKGROUNDS_UPDATED, backgrounds)

export const searchPromotionDisabled = () =>
  action(types.SEARCH_PROMOTION_DISABLED, {})
