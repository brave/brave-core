// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Reducer } from 'redux'

// Constants
import { types, DismissBrandedWallpaperNotificationPayload } from '../constants/new_tab_types'
import { Stats } from '../api/stats'
import { PrivateTabData } from '../api/privateTabData'

// API
import * as backgroundAPI from '../api/background'
import { InitialData } from '../api/initialData'
import { registerViewCount } from '../api/wallpaper'
import * as preferencesAPI from '../api/preferences'
import * as storage from '../storage/new_tab_storage'
import { setMostVisitedSettings } from '../api/topSites'

// Utils
import { handleWidgetPrefsChange } from './stack_widget_reducer'
import { NewTabAdsData } from '../api/newTabAdsData'
import { Background, CustomBackground } from '../api/background'

let sideEffectState: NewTab.State = storage.load()

type SideEffectFunction = (currentState: NewTab.State) => void

function performSideEffect (fn: SideEffectFunction): void {
  window.setTimeout(() => fn(sideEffectState), 0)
}

export const newTabReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State, action) => {
  const payload = action.payload

  switch (action.type) {
    case types.NEW_TAB_SET_INITIAL_DATA:
      const initialDataPayload = payload as InitialData
      state = {
        ...state,
        initialDataLoaded: true,
        ...initialDataPayload.preferences,
        stats: initialDataPayload.stats,
        ...initialDataPayload.privateTabData,
        braveRewardsSupported: initialDataPayload.braveRewardsSupported,
        braveTalkSupported: initialDataPayload.braveTalkSupported,
        searchPromotionEnabled: initialDataPayload.searchPromotionEnabled,
        customImageBackgrounds: initialDataPayload.customImageBackgrounds
      }

      if (initialDataPayload.wallpaperData) {
        let backgroundWallpaper = initialDataPayload.wallpaperData.backgroundWallpaper
        if (backgroundWallpaper?.type === 'color' && backgroundWallpaper.random) {
          backgroundWallpaper = backgroundAPI.randomColorBackground(backgroundWallpaper.wallpaperColor)
        } else if (backgroundWallpaper?.type === 'image' && backgroundWallpaper.random) {
          const customBackgrounds = state.customImageBackgrounds
          if (customBackgrounds.length) {
            backgroundWallpaper = { ...customBackgrounds[Math.floor(Math.random() * customBackgrounds.length)], random: true }
          }
        }

        state = {
          ...state,
          backgroundWallpaper,
          brandedWallpaper: initialDataPayload.wallpaperData.brandedWallpaper
        }
      }

      // It's super referral when background is false and it's not sponsored.
      if (state.brandedWallpaper && !state.brandedWallpaper.isSponsored) {
        // Update feature flag if this is super referral wallpaper.
        state = {
          ...state,
          featureFlagBraveNTPSponsoredImagesWallpaper: false
        }
      }
      // Set default if we can't get both.
      if (!state.backgroundWallpaper && !state.brandedWallpaper) {
        state.backgroundWallpaper = backgroundAPI.randomBackgroundImage()
      }
      console.timeStamp('reducer initial data received')

      performSideEffect(async function (state) {
        if (!state.isIncognito) {
          try {
            await registerViewCount()
          } catch (e) {
            console.error('Error calling registerViewCount', e)
          }
        }
      })

      state = storage.addNewStackWidget(state)
      state = storage.replaceStackWidgets(state)

      break

    case types.NEW_TAB_STATS_UPDATED:
      const stats: Stats = payload.stats
      state = {
        ...state,
        stats
      }
      break

    case types.SEARCH_PROMOTION_DISABLED:
      state = {
        ...state,
        searchPromotionEnabled: false
      }
      break

    case types.BACKGROUND_UPDATED:
      // While customizing background, background has
      // custom or brave default background. Branded wallpaper will
      // be visible after reloading or new NTP after changing custom
      // background option.
      state = {
        ...state,
        brandedWallpaper: undefined,
        backgroundWallpaper: undefined
      }

      const background = payload.background as Background
      if (background?.custom) {
        const url = background.custom.url.url
        const color = background.custom.color
        const random = background.custom.useRandomItem
        if (color) {
          state.backgroundWallpaper = random ? backgroundAPI.randomColorBackground(color) : { type: 'color', wallpaperColor: color, random }
        } else if (url) {
          // Custom Image was specified
          state.backgroundWallpaper = { type: 'image', wallpaperImageUrl: url }
        } else if (random) {
          // Random custom image should be used.
          const customBackgrounds = state.customImageBackgrounds
          if (customBackgrounds.length) {
            state.backgroundWallpaper = { ...customBackgrounds[Math.floor(Math.random() * customBackgrounds.length)], random: true }
          }
        }
      }

      if (!state.backgroundWallpaper && background?.brave) {
        state.backgroundWallpaper = {
          type: 'brave',
          author: background.brave.author,
          link: background.brave.link.url,
          wallpaperImageUrl: background.brave.imageUrl.url,
          random: false
        }
      }

      if (!state.backgroundWallpaper) {
        state.backgroundWallpaper = backgroundAPI.randomBackgroundImage()
      }
      break

    case types.CUSTOM_IMAGE_BACKGROUNDS_UPDATED:
      const customBackgrounds = payload as CustomBackground[]
      state = {
        ...state,
        customImageBackgrounds: customBackgrounds.map(background => ({ type: 'image', wallpaperImageUrl: background.url.url }))
      }
      break

    case types.NEW_TAB_PRIVATE_TAB_DATA_UPDATED:
      const privateTabData = payload as PrivateTabData
      state = {
        ...state,
        useAlternativePrivateSearchEngine: privateTabData.useAlternativePrivateSearchEngine,
        showAlternativePrivateSearchEngineToggle: privateTabData.showAlternativePrivateSearchEngineToggle
      }
      break

    case types.NEW_TAB_ADS_DATA_UPDATED:
      const newTabAdsData = payload as NewTabAdsData
      state.rewardsState.needsBrowserUpgradeToServeAds = newTabAdsData.needsBrowserUpgradeToServeAds
      break

    case types.NEW_TAB_DISMISS_BRANDED_WALLPAPER_NOTIFICATION:
      const { isUserAction } = payload as DismissBrandedWallpaperNotificationPayload
      // Save persisted data.
      preferencesAPI.saveIsBrandedWallpaperNotificationDismissed(true)
      // Only change current data if user explicitly took an action (e.g. clicked
      // on the "Close notification Button" - x).
      if (isUserAction) {
        state = {
          ...state,
          isBrandedWallpaperNotificationDismissed: true
        }
      }
      break

    case types.NEW_TAB_PREFERENCES_UPDATED:
      const preferences = payload as NewTab.Preferences
      const newState = {
        ...state,
        ...preferences
      }
      // We don't want to update dismissed status of branded wallpaper notification
      // since this can happen automatically when the notification is counted as
      // 'viewed', but we want to keep showing it until the page is navigated away from
      // or refreshed.
      newState.isBrandedWallpaperNotificationDismissed = state.isBrandedWallpaperNotificationDismissed
      // Remove branded wallpaper when opting out or turning wallpapers off
      const hasTurnedBrandedWallpaperOff = !preferences.brandedWallpaperOptIn && state.brandedWallpaper
      const hasTurnedWallpaperOff = !preferences.showBackgroundImage && state.showBackgroundImage
      // We always show SR images regardless of background options state.
      const isSuperReferral = state.brandedWallpaper && !state.brandedWallpaper.isSponsored
      if (!isSuperReferral &&
          (hasTurnedBrandedWallpaperOff || (state.brandedWallpaper && hasTurnedWallpaperOff))) {
        newState.brandedWallpaper = undefined
      }
      // Get a new wallpaper image if turning that feature on
      const shouldChangeBackgroundImage =
        !state.showBackgroundImage && preferences.showBackgroundImage
      if (shouldChangeBackgroundImage && !newState.backgroundWallpaper) {
        newState.backgroundWallpaper = backgroundAPI.randomBackgroundImage()
      }
      // Handle updated widget prefs
      state = handleWidgetPrefsChange(newState, state)
      break

    case types.SET_MOST_VISITED_SITES: {
      const { showTopSites, customLinksEnabled } = payload
      performSideEffect(async function (state) {
        setMostVisitedSettings(customLinksEnabled, showTopSites)
      })
      break
    }

    case types.TOP_SITES_STATE_UPDATED: {
      const { newShowTopSites, newCustomLinksEnabled, customLinksNum } = payload
      state = {
        ...state,
        showTopSites: newShowTopSites,
        customLinksEnabled: newCustomLinksEnabled,
        customLinksNum: customLinksNum
      }
      break
    }

    case types.CUSTOMIZE_CLICKED: {
      performSideEffect(async function (state) {
        chrome.send('customizeClicked', [])
      })
      break
    }

    default:
      break
  }

  sideEffectState = state
  return state
}

export default newTabReducer
