/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';

import * as PageVisibility from './page_visibility.js'

function modifyPageVisibity () {
  // Use chromium value defined in page_visibility.js in guest mode
  // which hides most sections.
  if (!loadTimeData.getBoolean('isGuest')) {
    return
  }

  // Sections will only be hidden if the explicitly `=== false`.
  // Therefore we only need to modify or add new items here if we want to
  // hide them (always or conditionally).

  const bravePageVisibility = PageVisibility.pageVisibility

  // add sections
  bravePageVisibility.socialBlocking = true
  bravePageVisibility.braveSync = !loadTimeData.getBoolean('isSyncDisabled')

  // hide sections
  bravePageVisibility.a11y = false
  bravePageVisibility.people = false
  bravePageVisibility.defaultBrowser = false

  PageVisibility.setPageVisibilityForTesting(bravePageVisibility)
}

modifyPageVisibity()
