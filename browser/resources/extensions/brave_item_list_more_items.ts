/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Convert to Polymer class and remove ts-nocheck

import {I18nBehavior} from 'chrome://resources/cr_elements/i18n_behavior.js';
import {Polymer} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import 'chrome://resources/cr_elements/cr_shared_style.css.js'
import 'chrome://resources/js/cr.m.js'
import {getTemplate} from './brave_item_list_more_items.html.js'

Polymer({
  is: 'extensions-brave-item-list-more-items',

  _template: getTemplate(),

  behaviors: [I18nBehavior],

  properties: {
    /** @type {!Array<!chrome.developerPrivate.ExtensionInfo>} */
    apps: Array,

    /** @type {!Array<!chrome.developerPrivate.ExtensionInfo>} */
    extensions: Array,
  },

  shouldShowMoreItemsMessage_: function() {
    if (!this.apps || !this.extensions)
      return;

    return this.apps.length !== 0 || this.extensions.length !== 0;
  },
});
