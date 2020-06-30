// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {BraveClearBrowsingDataOnExitBehavior} from '../brave_clear_browsing_data_dialog/brave_clear_browsing_data_dialog_behavior.js'
import {RegisterPolymerComponentBehaviors} from 'chrome://brave-resources/polymer_overriding.js'

RegisterPolymerComponentBehaviors({
  'settings-clear-browsing-data-dialog': [
    BraveClearBrowsingDataOnExitBehavior
  ]
})
