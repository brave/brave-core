// @ts-nocheck
/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import {loadTimeData} from "../i18n_setup.js"

const BraveAddSiteDialogBehaviorImpl = {
  ready: function() {
    const is_brave_shields = this.category === 'braveShields'
    const resource_id = is_brave_shields ? 'braveShieldsExampleTemplate' : 'addSiteExceptionPlaceholder'
    if (!this.$.site) {
      console.error('[Brave Settings Overrides] site input field not found')
      return
    }
    this.$.site.placeholder = loadTimeData.getString(resource_id)
  },
}

export const BraveAddSiteDialogBehavior = [
  BraveAddSiteDialogBehaviorImpl
]
