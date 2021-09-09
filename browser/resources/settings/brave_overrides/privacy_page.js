// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js'

RegisterPolymerTemplateModifications({
  'settings-privacy-page': (templateContent) => {
    const pages = templateContent.getElementById('pages')
    if (!pages) {
      console.error(`[Brave Settings Overrides] Couldn't find privacy_page #pages`)
    } else {
      pages.insertAdjacentHTML('beforeend', `
        <template is="dom-if" route-path="/content/autoplay" no-search>
          <settings-subpage page-title="${I18nBehavior.i18n('siteSettingsCategoryAutoplay')}">
            <category-default-setting
                toggle-off-label="${I18nBehavior.i18n('siteSettingsBlocked')}"
                toggle-on-label="${I18nBehavior.i18n('siteSettingsAllowed')}"
                category="[[contentSettingsTypesEnum_.AUTOPLAY}}">
            </category-default-setting>
            <category-setting-exceptions
                category="[[contentSettingsTypesEnum_.AUTOPLAY]]"
                block-header="${I18nBehavior.i18n('siteSettingsBlock')}">
            </category-setting-exceptions>
          </settings-subpage>
        </template>
      `)
      const isNativeBraveWalletEnabled = loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')
      if (isNativeBraveWalletEnabled) {
        pages.insertAdjacentHTML('beforeend', `
          <template is="dom-if" route-path="/content/ethereum" no-search>
          <settings-subpage page-title="${I18nBehavior.i18n('siteSettingsCategoryEthereum')}">
          <category-default-setting
          category="[[contentSettingsTypesEnum_.ETHEREUM]]"
          toggle-off-label="${I18nBehavior.i18n('siteSettingsEthereumBlock')}"
          toggle-on-label="${I18nBehavior.i18n('siteSettingsEthereumAsk')}">
          </category-default-setting>
          <category-setting-exceptions
          category="[[contentSettingsTypesEnum_.ETHEREUM]]"
          read-only-list
          block-header="${I18nBehavior.i18n('siteSettingsBlock')}"
          allow-header="${I18nBehavior.i18n('siteSettingsAllow')}">
          </category-setting-exceptions>
          </settings-subpage>
          </template>
        `)
      }
    }
  },
})
