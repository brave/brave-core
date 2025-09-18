// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {loadTimeData} from 'chrome://resources/js/load_time_data.js'

import './brave_account_row.js'
import '../people_page/people_page.js'
import '../settings_page/settings_section.js'
import '../default_browser_page/default_browser_page.js'
import '../on_startup_page/on_startup_page.js'
import {getTemplate} from './getting_started.html.js'

import {SettingsViewMixin, SettingsViewMixinInterface} from '../settings_page/settings_view_mixin.js'

// <if expr="enable_pin_shortcut">
import '../pin_shortcut_page/pin_shortcut_page.js'
// </if>


export class BraveSettingsGettingStarted extends SettingsViewMixin(PolymerElement) {
  static get is() {
    return 'brave-settings-getting-started'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      isBraveAccountEnabled_: {
        type: Boolean,
        value: () => loadTimeData.getBoolean('isBraveAccountEnabled'),
      },
    }
  }

  declare private isBraveAccountEnabled_: boolean

  override getAssociatedControlFor(childViewId: string): HTMLElement {
    switch (childViewId) {
      case 'manageProfile':
        return this.shadowRoot!.querySelector('settings-people-page')!.shadowRoot!.querySelector('#edit-profile')!;
      default:
        throw new Error(`Unknown child view id: ${childViewId}`)
    }
  }
}

customElements.define(BraveSettingsGettingStarted.is, BraveSettingsGettingStarted);
