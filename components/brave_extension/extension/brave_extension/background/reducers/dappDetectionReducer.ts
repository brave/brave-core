/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as webNavigationTypes from '../../constants/webNavigationTypes'
import { isHttpOrHttps } from '../../helpers/urlUtils'
import { Actions } from '../../types/actions/index'

export default function dappDetectionReducer (state = {}, action: Actions) {
  switch (action.type) {
    case webNavigationTypes.ON_COMMITTED: {
      if (chrome.braveWallet && action.isMainFrame && isHttpOrHttps(action.url)) {
        chrome.braveWallet.shouldCheckForDapps((dappDetection) => {
          if (!dappDetection) {
            return
          }
          chrome.tabs.executeScript(action.tabId, {
            file: 'out/content_dapps.bundle.js',
            allFrames: false,
            runAt: 'document_start',
            frameId: 0
          }, () => {
            if (chrome.runtime.lastError) {
              console.warn('Dapp detection inject via execute script received an error:', chrome.runtime.lastError)
            }
          })
        })
      }
      break
    }
  }
  return state
}
