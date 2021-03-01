/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';

import {pageVisibility as chromiumPageVisibility, setPageVisibilityForTesting} from '../page_visibility.js'

const alwaysTrue = {
  get: () => true
}

const alwaysTrueProxy = new Proxy({}, alwaysTrue)

function getPageVisibility () {
  // Use chromium value defined in page_visibility.js in guest mode
  // which hides most sections, and add brave sections to hide.
  if (loadTimeData.getBoolean('isGuest')) {
    // Hide appropriate brave sections as well as chromium ones
    return {
      ...chromiumPageVisibility,
      shields: true,
      socialBlocking: true,
      braveSync: false,
      getStarted: false,
      newTab: false,
    }
  }
  // We need to specify values for every attribute in pageVisibility instead of
  // only overriding specific attributes here because chromium does not
  // explicitly define pageVisibility in page_visibility.js since polymer only
  // notifies after a property is set.
  // Use proxy objects here so we only need to write out the attributes we
  // would like to hide.
  // See brave/browser/settings/brave_overrides/basic_page.js for brave's list,
  // and chrome/browser/settings/page_visibility.js for chromium's list.
  const staticProps = {
    // future-proof chromium actually defining something,
    ...chromiumPageVisibility,
    // overrides
    a11y: false,
    people: false,
    defaultBrowser: false,
    onStartup: false,
    appearance: alwaysTrueProxy,
    privacy: alwaysTrueProxy,
    // custom properties
    braveSync: !loadTimeData.getBoolean('isSyncDisabled'),
  }
  // Proxy so we can respond to any other property
  return new Proxy(staticProps, {
    get: function(target, prop) {
      if (prop in target) {
        return target[prop]
      }
      // default to allow, like chromium
      return true
    }
  })
}
// Provide an export in case our overrides want to explicitly import this override
// Even though we are modifying chromium's override, the es module eval timing may
// result in the unoverridden value being obtained.
export const pageVisibility = getPageVisibility()
setPageVisibilityForTesting(pageVisibility)
