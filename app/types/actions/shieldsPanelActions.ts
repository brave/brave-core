import * as types from '../constants/shieldsPanelTypes'

export interface details {
  adBlock: string,
  trackingProtection: string,
  httpsEverywhere: string,
  origin: string,
  hostname: string
}

export type settings = 'allow' | 'block'

export interface shieldsPanelDataUpdated {
  (details: details): {
    type: types.SHIELDS_PANEL_DATA_UPDATED,
    details: details
  }
}

export interface shieldsToggled {
  (setting: settings): {
    type: types.SHIELDS_TOGGLED,
    setting: settings
  }
}

export interface adBlockToggled {
  (): {
    type: types.AD_BLOCK_TOGGLED
  }
}

export interface trackingProtectionToggled {
  (): {
    type: types.TRACKING_PROTECTION_TOGGLED
  }
}

export interface resourceBlocked {
  (details: details): {
    type: types.RESOURCE_BLOCKED,
    details: details
  }
}

export interface blockAdsTrackers {
  (setting: settings): {
    type: types.BLOCK_ADS_TRACKERS,
    setting: settings
  }
}

export interface controlsToggled {
  (setting: boolean): {
    type: types.CONTROLS_TOGGLED,
    setting: boolean
  }
}

export interface httpsEverywhereToggled {
  (): {
    type: types.HTTPS_EVERYWHERE_TOGGLED
  }
}

export interface javascriptToggled {
  (): {
    type: types.JAVASCRIPT_TOGGLED
  }
}

export type shieldPanelActions =
  shieldsPanelDataUpdated |
  shieldsToggled |
  adBlockToggled |
  trackingProtectionToggled |
  resourceBlocked |
  blockAdsTrackers |
  controlsToggled |
  httpsEverywhereToggled |
  javascriptToggled
