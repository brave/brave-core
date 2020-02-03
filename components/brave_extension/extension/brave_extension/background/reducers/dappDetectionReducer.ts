/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as webNavigationTypes from '../../constants/webNavigationTypes'
import { Actions } from '../../types/actions/index'
import { isBrowserUrl } from '../../helpers/urlUtils'

export default function dappDetectionReducer (state = {}, action: Actions) {
  switch (action.type) {
    case webNavigationTypes.ON_COMMITTED: {
      // Browser built in pages can't have scripts injected into them so ignore them
      if (chrome.braveWallet && action.isMainFrame && !isBrowserUrl(action.url)) {
        chrome.braveWallet.shouldCheckForDapps((dappDetection) => {
          if (!dappDetection) {
            return
          }
          chrome.tabs.executeScript(action.tabId, {
            file: 'out/content_dapps.bundle.js',
            allFrames: false,
            runAt: 'document_start',
            frameId: 0
          })
        })
      }
      break
    }
  }
  return state
}
