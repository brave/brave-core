/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const BraveResetProfileDialogBehaviorImpl = {
  ready: function() {
    this.fixSendSettingsCheckbox()
  },

  /**
   * Unchecks, disables, and hides sendSettings checkbox.
   * Also, hide the entire footer.
   * @private
   */
  fixSendSettingsCheckbox: function() {
    const sendSettings = (this as unknown as HTMLElement).shadowRoot!.getElementById('sendSettings') as HTMLInputElement
    sendSettings.checked = false
    sendSettings.hidden = true
    sendSettings.disabled = true
    sendSettings.parentNode && ((sendSettings.parentNode as HTMLElement).hidden = true)
  }
}

export const BraveResetProfileDialogBehavior = [
  BraveResetProfileDialogBehaviorImpl
]
