/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 /// <reference path="../../../node_modules/@types/chrome/index.d.ts" />

type BlockTypes = 'adBlock' | 'trackingProtection' | 'httpsEverywhere' | 'javascript'

interface BraveAdBlock {
  setAsync: any
  getAsync: any
}

interface BraveTrackingProtection {
  setAsync: any
  getAsync: any
}

interface BraveHTTPSEverywhere {
  setAsync: any
  getAsync: any
}

interface BraveAdBlock {
  setAsync: any
  getAsync: any
}

interface BraveTrackingProtection {
  getAsync: any
}

interface BlockDetails {
  blockType: BlockTypes
  tabId: number
}

declare namespace chrome.tabs {
  const setAsync: any
  const getAsync: any
}

declare namespace chrome.windows {
  const getAllAsync: any
}

declare namespace chrome.contentSettings {
  const braveAdBlock: BraveAdBlock
  const braveTrackingProtection: BraveTrackingProtection
  const braveHTTPSEverywhere: BraveHTTPSEverywhere

  interface ContentSetting {
    setAsync: any
    getAsync: any
  }
}

declare namespace chrome.braveShields {
  const onBlocked: {
    addListener: (callback: (detail: BlockDetails) => void) => void
  }
}
