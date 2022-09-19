// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {loadTimeData} from "../i18n_setup.js"
import '../settings_shared.css.js';
import '../settings_vars.css.js';
import { I18nBehavior } from 'chrome://resources/cr_elements/i18n_behavior.js'

/**
 * 'settings-brave-appearance-toolbar' is the settings page area containing
 * brave's appearance settings related to the toolbar.
 */
Polymer({
  is: 'settings-brave-appearance-toolbar',

  _template: html`{__html_template__}`,

  behaviors: [I18nBehavior],

  properties: {
    tabTooltipModes_: {
      readOnly: true,
      type: Array,
      value() {
        return [
          { value: 1, name: this.i18n('appearanceSettingsTabHoverModeCard') },
          {
            value: 2,
            name: this.i18n('appearanceSettingsTabHoverModeCardWithPreview')
          },
          { value: 0, name: this.i18n('appearanceSettingsTabHoverModeTooltip') }
        ]
      }
    }
  },

  showBraveVPNOption_: function () {
    return loadTimeData.getBoolean('isBraveVPNEnabled')
  }
})
