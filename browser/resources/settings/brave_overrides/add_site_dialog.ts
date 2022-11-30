// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterPolymerComponentBehaviors} from 'chrome://resources/polymer_overriding.js'
import {BraveAddSiteDialogBehavior} from '../brave_add_site_dialog/brave_add_site_dialog_behavior.js'

RegisterPolymerComponentBehaviors({
  'add-site-dialog': [
    BraveAddSiteDialogBehavior
  ]}
)

