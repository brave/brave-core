/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

cr.define('extensions', function() {
  'use strict';

  const MoreItems = Polymer({
    is: 'extensions-brave-item-list-more-items',

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

  return {MoreItems: MoreItems};
});
