/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { combineReducers } from 'redux'

// Utils
import webcompatReporterReducer from './webcompatreporter_reducer'

export default combineReducers<WebcompatReporter.ApplicationState>({
  reporterState: webcompatReporterReducer
})
