/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 /// <reference path="../../../node_modules/@types/chrome/index.d.ts" />

type BlockTypes = 'ads' | 'trackers' | 'httpUpgradableResources' | 'javascript' | 'fingerprinting'

interface BlockDetails {
  blockType: BlockTypes
  tabId: number
  subresource: string
}

interface BlockDetails {
  blockType: BlockTypes
  tabId: number
  subresource: string
}
declare namespace chrome.tabs {
  const setAsync: any
  const getAsync: any
}

declare namespace chrome.windows {
  const getAllAsync: any
}

declare namespace chrome.braveShields {
  const onBlocked: {
    addListener: (callback: (detail: BlockDetails) => void) => void
    emit: (detail: BlockDetails) => void
  }

  const allowScriptsOnce: any
  const javascript: any
  const plugins: any
}
