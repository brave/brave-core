/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Tab } from '../types/state/shieldsPannelState'
import { BlockOptions } from '../types/other/blockTypes'
import { getLocale } from '../background/api/localeAPI'

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

export const getFavicon = (url: string) => {
  return `chrome://favicon/size/16@1x/${url}`
}

export const blockedResourcesSize = (blockedResources: number) => {
  if (blockedResources > 99) {
    return '99+'
  }
  if (!blockedResources) {
    return '0'
  }
  return blockedResources.toString()
}

export const isShieldsEnabled = (braveShields: BlockOptions) => braveShields !== 'block'

export const getTabIndexValueBasedOnProps = (
  isBlockedListOpen: boolean,
  numberOfBlockedItems: number
): number => {
  return (isBlockedListOpen || numberOfBlockedItems === 0) ? -1 : 0
}

export const getTotalBlockedSizeStrings = (blockedItems: number, httpsUpgrades: number) => {
  if (blockedItems === 0 && httpsUpgrades === 0) {
    return `${getLocale('itemsBlocked')} ${getLocale('and')} ${getLocale('connectionsUpgraded')}`
  } else if (blockedItems === 1 && httpsUpgrades === 0) {
    return getLocale('itemBlocked')
  } else if (blockedItems === 0 && httpsUpgrades === 1) {
    return getLocale('connectionUpgradedHTTPS')
  } else if (blockedItems > 1 && httpsUpgrades === 0) {
    return getLocale('itemsBlocked')
  } else if (blockedItems === 0 && httpsUpgrades > 1) {
    return getLocale('connectionsUpgradedHTTPS')
  } else {
    return `${getLocale('itemsBlocked')} ${getLocale('and')} ${getLocale('connectionsUpgraded')}`
  }
}

export const getToggleStateViaEventTarget = (event: React.ChangeEvent<HTMLInputElement>) => {
  return event.target.checked ? 'allow' : 'block'
}

export const maybeBlockResource = (resouce: BlockOptions) => {
  return resouce !== 'allow'
}

export const shouldDisableResourcesRow = (resource: number) => {
  return resource === 0
}

export const sumAdsAndTrackers = (ads: number, trackers: number) => {
  return ads + trackers
}

export const mergeAdsAndTrackersResources = (ads: string[], trackers: string[]) => {
  return [...ads, ...trackers]
}

export const shieldsHasFocus = (url: string) => {
  const devtoolsURL: string =
    'chrome-extension://mnojpmjdmbbfmejpflffifhffcmidifd'
  // Consider Shields to be focused if there's real focus
  // or the focused window is a devtools window.
  return (
    document.hasFocus() ||
    url.startsWith(devtoolsURL)
  )
}
