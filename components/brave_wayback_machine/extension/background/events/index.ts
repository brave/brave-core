/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { statusCode } from '../../utils'
import { shouldRedirectUsers } from '../actions'

chrome.webRequest.onHeadersReceived.addListener(details => {
  // only proceed this call if header response is something we care about
  if (!statusCode.includes(details.statusCode)) {
    return
  }

  const confirmMessage = confirm('an archived version of this website is available. do you want to load it?')
  if (confirmMessage) {
    shouldRedirectUsers(details.tabId, details.url)
  }
},
  {
    urls: [ '<all_urls>' ],
    types: ['main_frame']
  },
  ['blocking']
)
