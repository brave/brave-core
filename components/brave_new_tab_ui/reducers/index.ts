// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { combineReducers } from 'redux'

// Reducers
import newTabReducer from './new_tab_reducer'
import gridSitesReducer from './grid_sites_reducer'

export default combineReducers<NewTab.ApplicationState>({
  newTabData: newTabReducer,
  gridSitesData: gridSitesReducer
})
