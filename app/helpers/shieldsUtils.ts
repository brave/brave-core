/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Tab } from '../types/state/shieldsPannelState'

export const getTotalResourcesBlocked = (tabData: Partial<Tab>) => {
  if (!tabData) {
    return 0
  }
  return (
    tabData.adsBlocked! +
    tabData.trackersBlocked! +
    tabData.javascriptBlocked! +
    tabData.fingerprintingBlocked! +
    tabData.httpsRedirected!
  )
}

export const totalAdsTrackersBlocked = (adsBlocked: number, trackersBlocked: number) => {
  return adsBlocked + trackersBlocked
}

export const getFavicon = (url: string) => {
  return `chrome://favicon/size/16@1x/${ url }`
}
