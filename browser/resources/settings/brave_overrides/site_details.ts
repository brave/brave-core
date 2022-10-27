// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {RegisterPolymerTemplateModifications} from 'chrome://resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/i18n_behavior.js'
import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';

RegisterPolymerTemplateModifications({
  'site-details': (templateContent) => {
    if (!loadTimeData.getBoolean('isIdleDetectionFeatureEnabled')) {
      const idleDetectionItem = templateContent.querySelector('[category="[[contentSettingsTypesEnum_.IDLE_DETECTION]]"]')
      if (!idleDetectionItem) {
        console.error(`[Brave Settings Overrides] Couldn't find idle detection item`)
      } else {
        idleDetectionItem.hidden = true
      }
    }
    const firstPermissionItem = templateContent.querySelector('div.list-frame > site-details-permission:nth-child(1)')
    if (!firstPermissionItem) {
      console.error(`[Brave Settings Overrides] Couldn't find first permission item`)
    } else {
      firstPermissionItem.insertAdjacentHTML('beforebegin', `
        <site-details-permission
            category="[[contentSettingsTypesEnum_.AUTOPLAY]]"
            icon="cr:extension" label="${I18nBehavior.i18n('siteSettingsAutoplay')}">
        </site-details-permission>
      `)
      const isNativeBraveWalletEnabled = loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')
      if (isNativeBraveWalletEnabled) {
          firstPermissionItem.insertAdjacentHTML('beforebegin', `
          <site-details-permission
              category="[[contentSettingsTypesEnum_.ETHEREUM]]"
              icon="cr:extension" label="${I18nBehavior.i18n('siteSettingsEthereum')}">
          </site-details-permission>
        `)
          firstPermissionItem.insertAdjacentHTML('beforebegin', `
          <site-details-permission
              category="[[contentSettingsTypesEnum_.SOLANA]]"
              icon="cr:extension" label="${I18nBehavior.i18n('siteSettingsSolana')}">
          </site-details-permission>
        `)
      }
    }
    const usageSection = templateContent.querySelector('div#usage')    
    if (!usageSection) {
      console.error(`[Brave Settings Overrides] Couldn't find usageSection item`)
    } else {
      usageSection.insertAdjacentHTML('afterend', `
        <div id="shields">
          <div id="shieldsHeader" style="padding: 0 var(--cr-section-padding);">
            <h2 class="first">${I18nBehavior.i18n('siteSettingsShields')}</h2>
          </div>
          <div class="list-frame">
            <site-details-permission
                category="[[contentSettingsTypesEnum_.BRAVE_SHIELDS]]"
                icon="brave_settings:shields" label="${I18nBehavior.i18n('siteSettingsShieldsStatus')}">
            </site-details-permission>
          </div>
        </div>
      `)
    }
  },
})
