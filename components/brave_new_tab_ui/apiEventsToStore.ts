// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import getActions from './api/getActions'
import * as preferencesAPI from './api/preferences'
import * as statsAPI from './api/stats'
import * as privateTabDataAPI from './api/privateTabData'
import getInitialData from './api/initialData'

async function updatePreferences (prefData: preferencesAPI.Preferences) {
  getActions().preferencesUpdated(prefData)
}

async function updateStats (statsData: statsAPI.Stats) {
  getActions().statsUpdated(statsData)
}

async function updatePrivateTabData (data: privateTabDataAPI.PrivateTabData) {
  getActions().privateTabDataUpdated(data)
}

// Not marked as async so we don't return a promise
// and confuse callers
export default function wireApiEventsToStore () {
  // Get initial data and dispatch to store
  getInitialData()
  .then((initialData) => {
    getActions().setInitialData(initialData)
    // Listen for API changes and dispatch to store
    statsAPI.addChangeListener(updateStats)
    preferencesAPI.addChangeListener(updatePreferences)
    privateTabDataAPI.addChangeListener(updatePrivateTabData)
  })
  .catch(e => {
    console.error('New Tab Page fatal error:', e)
  })
}
