// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as preferencesAPI from './preferences'
import * as statsAPI from './stats'
import * as wallpaper from './wallpaper'
import getNTPBrowserAPI from './background'
import getVPNServiceHandler, * as BraveVPN from '../api/braveVpn'
import { loadTimeData } from '$web-common/loadTimeData'

export type InitialData = {
  preferences: NewTab.Preferences
  stats: statsAPI.Stats
  wallpaperData?: NewTab.Wallpaper
  braveBackgrounds: NewTab.BraveBackground[]
  customImageBackgrounds: NewTab.ImageBackground[]
  braveRewardsSupported: boolean
  braveTalkSupported: boolean
  searchPromotionEnabled: boolean
  purchasedState: BraveVPN.PurchasedState
}

const isIncognito: boolean = chrome.extension.inIncognitoContext

// Gets all data required for the first render of the page
export async function getInitialData (): Promise<InitialData> {
  try {
    console.timeStamp('Getting initial data...')
    const [
      preferences,
      stats,
      wallpaperData,
      braveRewardsSupported,
      braveTalkSupported,
      searchPromotionEnabled,
      braveBackgrounds,
      customImageBackgrounds,
      purchasedState
    ] = await Promise.all([
      preferencesAPI.getPreferences(),
      statsAPI.getStats(),
      !isIncognito ? wallpaper.getWallpaper() : Promise.resolve(undefined),
      false,
      new Promise((resolve) => {
        if (!('braveTalk' in chrome)) {
          resolve(false)
          return
        }

        chrome.braveTalk.isSupported((supported: boolean) => {
          resolve(supported)
        })
      }),
      getNTPBrowserAPI().pageHandler.isSearchPromotionEnabled().then(({ enabled }) => enabled),
      getNTPBrowserAPI().pageHandler.getBraveBackgrounds().then(({ backgrounds }) => {
        return backgrounds.map(background => ({ type: 'brave', wallpaperImageUrl: background.imageUrl.url, author: background.author, link: background.link.url }))
      }),
      getNTPBrowserAPI().pageHandler.getCustomImageBackgrounds().then(({ backgrounds }) => {
        return backgrounds.map(background => ({ type: 'image', wallpaperImageUrl: background.url.url }))
      }),
      new Promise((resolve) => {
        if (loadTimeData.getBoolean('vpnWidgetSupported')) {
          getVPNServiceHandler().getPurchasedState().then(({state}) => {
            resolve(state.state)
          })
        } else {
          resolve(BraveVPN.PurchasedState.NOT_PURCHASED)
        }
      })
    ])
    console.timeStamp('Got all initial data.')
    return {
      preferences,
      stats,
      wallpaperData,
      braveBackgrounds,
      customImageBackgrounds,
      braveRewardsSupported,
      braveTalkSupported,
      searchPromotionEnabled,
      purchasedState
    } as InitialData
  } catch (e) {
    console.error(e)
    throw Error('Error getting initial data')
  }
}
