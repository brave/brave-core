/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  html,
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'
import {loadTimeData} from '../i18n_setup.js'

RegisterPolymerTemplateModifications({
  'settings-sync-controls': (templateContent) => {
    // Remove payment integration from settings-sync-controls
    const paymentIntegrationToggle = templateContent.querySelector(
      'cr-toggle[checked="{{syncPrefs.paymentsSynced}}"]'
    )
    if (!paymentIntegrationToggle) {
      console.error(
        '[Brave Settings Overrides] Could not find sync control payment toggle'
      )
      return
    }
    paymentIntegrationToggle.parentElement!.remove()

    // <if expr="enable_ai_chat">
    // Add AI Chat sync toggle if the feature is enabled.
    if (loadTimeData.getBoolean('isBraveSyncAIChatEnabled')) {
      const syncDataTypes =
        templateContent.querySelector('#sync-data-types')
      if (syncDataTypes) {
        syncDataTypes.appendChild(html`
          <div class="list-item"
              hidden="[[!syncPrefs.aiChatRegistered]]">
            <div id="aiChatCheckboxLabel">
              ${loadTimeData.getString('aiChatCheckboxLabel')}
            </div>
            <cr-policy-indicator indicator-type="userPolicy"
                hidden="[[!showPolicyIndicator_(syncStatus,
                      syncPrefs.aiChatManaged)]]">
            </cr-policy-indicator>
            <cr-toggle checked="{{syncPrefs.aiChatSynced}}"
                on-change="onSingleSyncDataTypeChanged_"
                disabled="[[disableTypeCheckBox_(syncStatus,
                    syncPrefs.syncAllDataTypes, syncPrefs.aiChatManaged,
                    syncStatus.disabled)]]"
                aria-labelledby="aiChatCheckboxLabel">
            </cr-toggle>
          </div>
        `)
      }
    }
    // </if>
  }
})
