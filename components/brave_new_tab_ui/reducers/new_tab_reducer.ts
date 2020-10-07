// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { Reducer } from 'redux'

// Constants
import { types, DismissBrandedWallpaperNotificationPayload } from '../constants/new_tab_types'
import { Stats } from '../api/stats'
import { PrivateTabData } from '../api/privateTabData'
import { TorTabData } from '../api/torTabData'

// API
import * as backgroundAPI from '../api/background'
import { InitialData } from '../api/initialData'
import { registerViewCount } from '../api/brandedWallpaper'
import * as preferencesAPI from '../api/preferences'
import * as storage from '../storage/new_tab_storage'

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
        brandedWallpaperData: initialDataPayload.brandedWallpaperData,
        ...initialDataPayload.privateTabData,
        ...initialDataPayload.torTabData,
        togetherSupported: initialDataPayload.togetherSupported,
        geminiSupported: initialDataPayload.geminiSupported
      }
      if (state.brandedWallpaperData && !state.brandedWallpaperData.isSponsored) {
        // Update feature flag if this is super referral wallpaper.
        state = {
          ...state,
          featureFlagBraveNTPSponsoredImagesWallpaper: false
        }
      }
      // TODO(petemill): only get backgroundImage if no sponsored background this time.
      // ...We would also have to set the value at the action
      // the branded wallpaper is turned off. Since this is a cheap string API
      // (no image will be downloaded), we can afford to leave this here for now.
      if (initialDataPayload.preferences.showBackgroundImage) {
        state.backgroundImage = backgroundAPI.randomBackgroundImage()
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

      if (state.currentStackWidget) {
        state = storage.migrateStackWidgetSettings(state)
      }
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

    case types.NEW_TAB_PRIVATE_TAB_DATA_UPDATED:
      const privateTabData = payload as PrivateTabData
      state = {
        ...state,
        useAlternativePrivateSearchEngine: privateTabData.useAlternativePrivateSearchEngine
      }
      break

    case types.NEW_TAB_TOR_TAB_DATA_UPDATED:
      const torTabData = payload as TorTabData
      state = {
        ...state,
        torCircuitEstablished: torTabData.torCircuitEstablished,
        torInitProgress: torTabData.torInitProgress
      }
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
      const preferences = payload as preferencesAPI.Preferences
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
      const hasTurnedBrandedWallpaperOff = !preferences.brandedWallpaperOptIn && state.brandedWallpaperData
      const hasTurnedWallpaperOff = !preferences.showBackgroundImage && state.showBackgroundImage
      // We always show SR images regardless of background options state.
      const isSuperReferral = state.brandedWallpaperData && !state.brandedWallpaperData.isSponsored
      if (!isSuperReferral &&
          (hasTurnedBrandedWallpaperOff || (state.brandedWallpaperData && hasTurnedWallpaperOff))) {
        newState.brandedWallpaperData = undefined
      }
      // Get a new wallpaper image if turning that feature on
      const shouldChangeBackgroundImage =
        !state.showBackgroundImage && preferences.showBackgroundImage
      if (shouldChangeBackgroundImage) {
        newState.backgroundImage = backgroundAPI.randomBackgroundImage()
      }
      state = newState
      break

    case types.REMOVE_STACK_WIDGET:
      const widget: NewTab.StackWidget = payload.widget
      let { removedStackWidgets, widgetStackOrder } = state

      if (!widgetStackOrder.length) {
        break
      }

      if (!removedStackWidgets.includes(widget)) {
        removedStackWidgets.push(widget)
      }

      state = {
        ...state,
        removedStackWidgets
      }
      break

    case types.SET_FOREGROUND_STACK_WIDGET:
      const frontWidget: NewTab.StackWidget = payload.widget
      let newWidgetStackOrder = state.widgetStackOrder

      newWidgetStackOrder = newWidgetStackOrder.filter((widget: NewTab.StackWidget) => {
        return widget !== frontWidget
      })

      newWidgetStackOrder.push(frontWidget)

      state = {
        ...state,
        widgetStackOrder: newWidgetStackOrder
      }
      break

    default:
      break
  }

  sideEffectState = state
  return state
}

export default newTabReducer
