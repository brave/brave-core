// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import '../settings_shared.css.js';
import '../settings_vars.css.js';
import {I18nBehavior} from 'chrome://resources/cr_elements/i18n_behavior.js'

/**
 * 'settings-brave-appearance-sidebar' is the settings page area containing
 * brave's sidebar settings in appearance settings.
 * This is separated from 'settings-brave-appearance-toolbar' because sidebar
 * option is located below the home button option.
 */
Polymer({
  is: 'settings-brave-appearance-sidebar',

  _template: html`{__html_template__}`,

  behaviors: [I18nBehavior],

  properties: {
    sidebarShowOptions_: {
      readOnly: true,
      type: Array,
      value() {
        return [
          {value: 0, name: this.i18n('appearanceSettingsShowOptionAlways')},
          {value: 1, name: this.i18n('appearanceSettingsShowOptionMouseOver')},
          {value: 3, name: this.i18n('appearanceSettingsShowOptionNever')},
        ];
      },
    },

    sidebarShowEnabledLabel_: {
      readOnly: false,
      type: String,
    },
  },

  observers: [
    'onShowOptionChanged_(prefs.brave.sidebar.sidebar_show_option.value)',
  ],

  computeSidebarShowOptionSubLabel_(option) {
    return option === 3 ? this.i18n('appearanceSettingsSidebarDisabledDesc')
                        : this.i18n('appearanceSettingsSidebarEnabledDesc');
  },

  onShowOptionChanged_() {
    this.sidebarShowEnabledLabel_ =
      this.computeSidebarShowOptionSubLabel_(this.getCurrentSidebarOption_());
  },

  getCurrentSidebarOption_() {
    return this.get('prefs.brave.sidebar.sidebar_show_option.value');
  },
});
