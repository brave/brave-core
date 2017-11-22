/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as newTabPageTypes from '../../constants/newTabPageTypes'
import {createTab} from '../api/tabsAPI'

export default function shieldsPanelReducer (state = {}, action) {
  switch (action.type) {
    case newTabPageTypes.SETTINGS_ICON_CLICKED:
      createTab({
        url: 'chrome://settings/'
      }).catch(() => {
        console.error('Could not create settings tab')
      })
      break
  }
  return state
}
