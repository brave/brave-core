// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/cr_elements/i18n_behavior.js'
import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';

RegisterPolymerTemplateModifications({
  'settings-privacy-page': (templateContent) => {
    const pages = templateContent.getElementById('pages')
    if (!pages) {
      console.error(`[Brave Settings Overrides] Couldn't find privacy_page #pages`)
    } else {
      if (!loadTimeData.getBoolean('isIdleDetectionFeatureEnabled')) {
        const idleDetection = templateContent.querySelector('[route-path="/content/idleDetection"]')
        if (!idleDetection) {
          console.error(`[Brave Settings Overrides] Couldn't find idle detection template`)
        } else {
          idleDetection.content.firstElementChild.hidden = true
        }
      }
      pages.insertAdjacentHTML('beforeend', `
        <template is="dom-if" route-path="/content/autoplay" no-search>
          <settings-subpage page-title="${I18nBehavior.i18n('siteSettingsCategoryAutoplay')}">
            <category-default-setting
                toggle-off-label="${I18nBehavior.i18n('siteSettingsAutoplayBlock')}"
                toggle-on-label="${I18nBehavior.i18n('siteSettingsAutoplayAllow')}"
                category="[[contentSettingsTypesEnum_.AUTOPLAY}}">
            </category-default-setting>
            <category-setting-exceptions
                category="[[contentSettingsTypesEnum_.AUTOPLAY]]"
                block-header="${I18nBehavior.i18n('siteSettingsBlock')}"
                allow-header="${I18nBehavior.i18n('siteSettingsAllow')}">
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
        pages.insertAdjacentHTML('beforeend', `
          <template is="dom-if" route-path="/content/solana" no-search>
          <settings-subpage page-title="${I18nBehavior.i18n('siteSettingsCategorySolana')}">
          <category-default-setting
          category="[[contentSettingsTypesEnum_.SOLANA]]"
          toggle-off-label="${I18nBehavior.i18n('siteSettingsSolanaBlock')}"
          toggle-on-label="${I18nBehavior.i18n('siteSettingsSolanaAsk')}">
          </category-default-setting>
          <category-setting-exceptions
          category="[[contentSettingsTypesEnum_.SOLANA]]"
          read-only-list
          block-header="${I18nBehavior.i18n('siteSettingsBlock')}"
          allow-header="${I18nBehavior.i18n('siteSettingsAllow')}">
          </category-setting-exceptions>
          </settings-subpage>
          </template>
        `)
      }
    }

    if (!loadTimeData.getBoolean('isPrivacySandboxRestricted')) {
      const privacySandboxTemplate = templateContent.querySelector(`template[if*='isPrivacySandboxRestricted_']`)
      if (!privacySandboxTemplate) {
        console.error('[Brave Settings Overrides] Could not find template with if*=isPrivacySandboxRestricted_ on privacy page.')
      } else {
        const privacySandboxLinkRow = privacySandboxTemplate.content.getElementById('privacySandboxLinkRow')
        if (!privacySandboxLinkRow) {
          console.error('[Brave Settings Overrides] Could not find privacySandboxLinkRow id on privacy page.')
        } else {
          privacySandboxLinkRow.setAttribute('hidden', 'true')
        }
        const privacySandboxLink = privacySandboxTemplate.content.getElementById('privacySandboxLink')
        if (!privacySandboxLink) {
          console.error('[Brave Settings Overrides] Could not find privacySandboxLink id on privacy page.')
        } else {
          privacySandboxTemplate.setAttribute('hidden', 'true')
        }
      }
    }

    const showPrivacyGuideEntryPointTemplate = templateContent.querySelector(`template[if*='showPrivacyGuideEntryPoint_']`)
    if (!showPrivacyGuideEntryPointTemplate) {
      console.error('[Brave Settings Overrides] Could not find template with if*=showPrivacyGuideEntryPoint_ on privacy page.')
    } else {
      const privacyGuideLinkRow = showPrivacyGuideEntryPointTemplate.content.getElementById('privacyGuideLinkRow')
      if (!privacyGuideLinkRow) {
        console.error('[Brave Settings Overrides] Could not find privacyGuideLinkRow id on privacy page.')
      } else {
        privacyGuideLinkRow.setAttribute('hidden', 'true')
      }
    }
  },
})
