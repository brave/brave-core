/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import actions from '../actions/shieldsPanelActions'
import { BlockDetails } from '../../types/actions/shieldsPanelActions'

if (chrome.braveShields) {
  chrome.braveShields.onBlocked.addListener((detail: BlockDetails) => {
    actions.resourceBlocked(detail)
  })
} else {
  console.log('chrome.braveShields not enabled')
}
