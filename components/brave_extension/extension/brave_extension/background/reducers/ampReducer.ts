/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as webNavigationTypes from '../../constants/webNavigationTypes'
import { Actions } from '../../types/actions/index'
import * as ampAPI from '../api/ampAPI'

export default function ampReducer (state = {}, action: Actions) {
  switch (action.type) {
    case webNavigationTypes.ON_COMMITTED: {
      if (action.isMainFrame) {
        ampAPI.getAMPNeutralized().then(ampNeutralized => {
          if (ampNeutralized) {
            chrome.tabs.executeScript(action.tabId, {
              file: 'out/content_noamp.bundle.js',
              allFrames: false,
              runAt: 'document_start',
              frameId: 0
            })
          }
        })
        .catch(() => console.warn('Failed to read AMP neutralizer preference'))
      }
      break
    }
  }
  return state
}
