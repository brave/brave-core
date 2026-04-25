// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { bindActionCreators, Dispatch } from 'redux'
import * as newTabActions from '../actions/new_tab_actions'
import * as gridSitesActions from '../actions/grid_sites_actions'
import * as rewardsActions from '../actions/rewards_actions'
import * as stackWidgetActions from '../actions/stack_widget_actions'
import * as todayActions from '../actions/today_actions'
import * as braveVPNActions from '../actions/brave_vpn_actions'
import { NewTabActions } from '../constants/new_tab_types'
import store from '../store'

/**
 * Cache action creators. Deprecated. Use actions individually and dispatch provided
 * by redux store.
 */
let actions: NewTabActions
export default function getActions () {
  if (actions) {
    return actions
  }
  actions = getActionsForDispatch(store.dispatch.bind(store))
  return actions
}

export function getActionsForDispatch (dispatch: Dispatch) {
  const allActions = Object.assign({}, newTabActions, stackWidgetActions, gridSitesActions, rewardsActions)
  return {
    ...bindActionCreators(allActions, dispatch),
    today: bindActionCreators(todayActions, dispatch),
    braveVPN: bindActionCreators(braveVPNActions, dispatch)
  }
}
