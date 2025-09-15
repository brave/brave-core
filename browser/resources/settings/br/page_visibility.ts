/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {loadTimeData} from 'chrome://resources/js/load_time_data.js'

import {
  type PageVisibility,
  pageVisibility as chromiumPageVisibility,
  resetPageVisibilityForTesting
} from '../page_visibility.js'

// Merge our interface additions with upstream's interface
declare module '../page_visibility' {
  export interface PageVisibility {
    braveSync?: boolean
    braveWallet?: boolean
    // <if expr="enable_containers">
    containers?: boolean
    // </if>
    content?: boolean
    getStarted?: boolean
    leoAssistant?: boolean
    leoPersonalization?: boolean
    leoModels?: boolean
    newTab?: boolean
    origin?: boolean
    playlist?: boolean
    shields?: boolean
    socialBlocking?: boolean
    speedreader?: boolean
    surveyPanelist?: boolean,
    braveTor?: boolean
  }
}

const alwaysTrue = {
  get: () => true
}

const alwaysTrueProxy = new Proxy({}, alwaysTrue)

function getPageVisibility () {
  // Use Chromium value defined in page_visibility.ts in guest mode
  // which hides most sections, and add Brave sections to hide.
  if (loadTimeData.getBoolean('isGuest')) {
    // Hide appropriate brave sections as well as chromium ones
    return {
      ...chromiumPageVisibility,
      braveSync: false,
      braveWallet: false,
      // <if expr="enable_containers">
      containers: false,
      // </if>
      content: false,
      getStarted: false,
      leoAssistant: false,
      leoPersonalization: false,
      leoModels: false,
      newTab: false,
      origin: false,
      playlist: false,
      shields: true,
      socialBlocking: true,
      speedreader: false,
      surveyPanelist: false,
      braveTor: false,
    }
  }
  // We need to specify values for every attribute in pageVisibility instead of
  // only overriding specific attributes here because Chromium does not
  // explicitly define pageVisibility in page_visibility.ts since Polymer only
  // notifies after a property is set.
  // Use proxy objects here so we only need to write out the attributes we
  // would like to hide.
  // See brave/browser/resources/settings/br/basic_page.ts for Brave's list,
  // and chrome/browser/resources/settings/page_visibility.ts for Chromium's list.
  const staticProps = {
    // future-proof chromium actually defining something,
    ...chromiumPageVisibility,
    // overrides
    people: false,
    defaultBrowser: false,
    onStartup: false,
    appearance: alwaysTrueProxy,
    privacy: alwaysTrueProxy,
    // custom properties
    braveSync: !loadTimeData.getBoolean('isSyncDisabled'),
    braveWallet: loadTimeData.getBoolean('isBraveWalletAllowed'),
    leoAssistant: loadTimeData.getBoolean('isLeoAssistantAllowed'),
    leoPersonalization: loadTimeData.getBoolean('isLeoAssistantAllowed'),
    leoModels: loadTimeData.getBoolean('isLeoAssistantAllowed'),
    surveyPanelist: loadTimeData.getBoolean('isSurveyPanelistAllowed'),
    // <if expr="enable_containers">
    containers: loadTimeData.getBoolean('isContainersEnabled'),
    // </if>
    content: alwaysTrueProxy,
    playlist: loadTimeData.getBoolean('isPlaylistAllowed'),
    speedreader: loadTimeData.getBoolean('isSpeedreaderAllowed'),
    braveTor: !loadTimeData.getBoolean('braveTorDisabledByPolicy') ||
              loadTimeData.getBoolean('shouldExposeElementsForTesting'),
    origin: loadTimeData.getBoolean('isOriginAllowed'),
  }
  // Proxy so we can respond to any other property
  return new Proxy(staticProps, {
    get: function(target, prop) {
      if (prop in target) {
        return target[prop as keyof Object]
      }
      // default to allow, like chromium
      return true
    }
  })
}

// Provide an export in case our overrides want to explicitly import this
// override. Even though we are modifying chromium's override, the es module
// eval timing may result in the unoverridden value being obtained.
export const pageVisibility = getPageVisibility()
resetPageVisibilityForTesting(pageVisibility as PageVisibility)
