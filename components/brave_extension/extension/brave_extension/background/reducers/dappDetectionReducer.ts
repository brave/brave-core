/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as webNavigationTypes from '../../constants/webNavigationTypes'
import { Actions } from '../../types/actions/index'

export default function dappDetectionReducer (state = {}, action: Actions) {
  switch (action.type) {
    case webNavigationTypes.ON_COMMITTED: {
      if (chrome.braveWallet && action.isMainFrame) {
        chrome.braveWallet.isEnabled((enabled) => {
          chrome.braveWallet.isInstalled((installed) => {
            if (installed || !enabled) {
              return
            }
            chrome.tabs.executeScript(action.tabId, {
              file: 'out/content_dapps.bundle.js',
              allFrames: false,
              runAt: 'document_start',
              frameId: 0
            })
          })
        })
      }
      break
    }
  }
  return state
}
