/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

BraveResetProfileDialogBehavior = {

  ready: function() {
    this.fixSendSettingsCheckbox();
  },

  /**
   * Unchecks, disables, and hides sendSettings checkbox.
   * Also, hide the entire footer.
   * @private
   */
  fixSendSettingsCheckbox: function() {
    this.$.sendSettings.checked = false;
    this.$.sendSettings.hidden = true;
    this.$.sendSettings.disabled = true;
    this.$.sendSettings.parentNode.hidden = true;
  },
};
