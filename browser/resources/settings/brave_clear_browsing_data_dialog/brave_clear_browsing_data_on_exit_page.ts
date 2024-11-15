// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck

import {Polymer} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {loadTimeData} from '../i18n_setup.js';
import '../settings_shared.css.js';
import '../settings_vars.css.js';
import '../controls/settings_checkbox.js';
import {getTemplate} from './brave_clear_browsing_data_on_exit_page.html.js'

Polymer({
  is: 'settings-brave-clear-browsing-data-on-exit-page',

  _template: getTemplate(),

  properties: {
    prefs: {
      type: Object,
      notify: true,
    },

    counters: {
      type: Object,
      // Will be filled as results are reported.
      value: function() {
        return {};
      }
    },

    isModified: {
      type: Boolean,
      value: false,
    },

    isChildAccount_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('isChildAccount');
      },
    },

    isAIChatAndHistoryAllowed_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('isLeoAssistantAllowed')
          && loadTimeData.getBoolean('isLeoAssistantHistoryAllowed');
      },
    }
  },

  listeners: {'settings-boolean-control-change': 'updateModified_'},

  setCounter: function (counter: string, text: string) {
    this.set('counters.' + counter, text);
  },

  getChangedSettings: function () {
    let changed: {key: string, value: boolean}[] = [];
    const boxes = this.$.checkboxes.querySelectorAll('settings-checkbox');
    boxes.forEach((checkbox) => {
      if (checkbox.checked != this.get(checkbox.pref.key, this.prefs).value) {
        changed.push({key:checkbox.pref.key, value:checkbox.checked});
      }
    });
    return changed;
  },

  updateModified_: function () {
    let modified = false;
    const boxes = this.$.checkboxes.querySelectorAll('settings-checkbox');
    for (let checkbox of boxes) {
      if (checkbox.checked != this.get(checkbox.pref.key, this.prefs).value) {
        modified = true;
        break;
      }
    }

    if (this.isModified !== modified) {
      this.isModified = modified;
      this.fire('clear-data-on-exit-page-change');
    }
  },
});
