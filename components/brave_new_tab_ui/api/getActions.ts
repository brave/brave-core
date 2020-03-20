/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { bindActionCreators } from 'redux'
import * as newTabActions from '../actions/new_tab_actions'
import store from '../store'

/**
 * Get actions from the C++ back-end down to front-end components
 */
let actions: typeof newTabActions
export default function getActions () {
  if (actions) {
    return actions
  }
  actions = bindActionCreators(newTabActions, store.dispatch.bind(store))
  return actions
}
