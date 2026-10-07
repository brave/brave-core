// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Brave Sync doesn't support payments, so drop its toggle and add one for AI
// Chat. isAiChatSyncEnabled_ and aiChatCheckboxLabel_ come from
// rewrite/.../sync_controls.ts.yaml.
mangle((root) => {
  const paymentsToggle = root.querySelector('#paymentsCheckbox')
  if (!paymentsToggle?.parentElement) {
    throw new Error(`[Settings] Sync controls: couldn't find #paymentsCheckbox`)
  }
  paymentsToggle.parentElement.remove()

  const syncDataTypes = root.querySelector('#sync-data-types')
  if (!syncDataTypes) {
    throw new Error(`[Settings] Sync controls: couldn't find #sync-data-types`)
  }
  syncDataTypes.insertAdjacentHTML(
    'beforeend',
    `\${this.isAiChatSyncEnabled_ ? html\`
      <div class="list-item"
          ?hidden="\${!this.syncPrefs?.aiChatRegistered}">
        <div id="aiChatCheckboxLabel">\${this.aiChatCheckboxLabel_}</div>
        <cr-policy-indicator indicator-type="userPolicy"
            ?hidden="\${!this.showPolicyIndicator_(
                this.syncPrefs?.aiChatManaged)}">
        </cr-policy-indicator>
        <cr-toggle .checked="\${!!this.syncPrefs?.aiChatSynced}"
            @change="\${this.onSingleSyncDataTypeChange_}"
            ?disabled="\${this.disableTypeCheckBox_(
                this.syncPrefs?.aiChatManaged)}"
            aria-labelledby="aiChatCheckboxLabel"
            data-type="\${UserSelectableType.AI_CHAT}"
            data-pref="aiChatSynced">
        </cr-toggle>
      </div>
    \` : ''}`)
})
