/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {loadTimeData} from "../i18n_setup.js"

import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {AddSiteDialogElement} from '../site_settings/add_site_dialog.js'

// Extend AddSiteDialog to change placeholder message for Shields.
const BaseElement = WebUiListenerMixin(AddSiteDialogElement)
export class BraveAddSiteDialogElement extends BaseElement {
  static override get properties() {
    const baseProperties = super.properties
    const extended = Object.assign({}, baseProperties, {
      sitePlaceholder: String
    })
    return extended
  }
  sitePlaceholder: string
  override ready() {
    super.ready()
    const is_brave_shields = this.category === 'braveShields'
    const resource_id = is_brave_shields ? 'braveShieldsExampleTemplate'
                                         : 'addSiteExceptionPlaceholder'
    this.sitePlaceholder = loadTimeData.getString(resource_id)
  }
}
