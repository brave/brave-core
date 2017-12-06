import * as types from '../constants/shieldsPanelTypes'

export interface Details {
  adBlock: string,
  trackingProtection: string,
  httpsEverywhere: string,
  origin: string,
  hostname: string
}

export type settings = 'allow' | 'block'

export interface ShieldsPanelDataUpdated {
  (details: Details): {
    type: types.SHIELDS_PANEL_DATA_UPDATED,
    details: Details
  }
}

export interface ShieldsToggled {
  (setting: settings): {
    type: types.SHIELDS_TOGGLED,
    setting: settings
  }
}

export interface AdBlockToggled {
  (): {
    type: types.AD_BLOCK_TOGGLED
  }
}

export interface TrackingProtectionToggled {
  (): {
    type: types.TRACKING_PROTECTION_TOGGLED
  }
}

export interface ResourceBlocked {
  (details: Details): {
    type: types.RESOURCE_BLOCKED,
    details: Details
  }
}

export interface BlockAdsTrackers {
  (setting: settings): {
    type: types.BLOCK_ADS_TRACKERS,
    setting: settings
  }
}

export interface ControlsToggled {
  (setting: boolean): {
    type: types.CONTROLS_TOGGLED,
    setting: boolean
  }
}

export interface HttpsEverywhereToggled {
  (): {
    type: types.HTTPS_EVERYWHERE_TOGGLED
  }
}

export interface JavascriptToggled {
  (): {
    type: types.JAVASCRIPT_TOGGLED
  }
}

export type shieldPanelActions =
  ShieldsPanelDataUpdated |
  ShieldsToggled |
  AdBlockToggled |
  TrackingProtectionToggled |
  ResourceBlocked |
  BlockAdsTrackers |
  ControlsToggled |
  HttpsEverywhereToggled |
  JavascriptToggled
