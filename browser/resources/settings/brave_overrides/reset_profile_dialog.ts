// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {BraveResetProfileDialogBehavior} from '../brave_reset_page/brave_reset_profile_dialog_behavior.js'
import {RegisterPolymerComponentBehaviors} from 'chrome://resources/polymer_overriding.js'

RegisterPolymerComponentBehaviors({
  'settings-reset-profile-dialog': [
    BraveResetProfileDialogBehavior
  ]
})
