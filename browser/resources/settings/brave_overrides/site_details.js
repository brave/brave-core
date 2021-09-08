// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js'

RegisterPolymerTemplateModifications({
  'site-details': (templateContent) => {
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
      }
    }
  },
})
