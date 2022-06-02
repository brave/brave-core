// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { RegisterPolymerComponentBehaviors, RegisterPolymerTemplateModifications } from 'chrome://brave-resources/polymer_overriding.js'
import { I18nBehavior } from 'chrome://resources/js/i18n_behavior.m.js'
import { loadTimeData } from 'chrome://resources/js/load_time_data.m.js'
import { routes } from '../route.js'
import { Router } from '../router.js'
import '../brave_privacy_page/brave_tor_subpage.js'


const TorOptionsBehaviour = {
  onTorClick_: function () {
    Router.getInstance().navigateTo(routes.TOR)
  },
}

RegisterPolymerComponentBehaviors({
  'settings-privacy-page': [
    TorOptionsBehaviour
  ]
})

RegisterPolymerTemplateModifications({
  'settings-privacy-page': (templateContent) => {
    const securityLinkRow = templateContent.getElementById('securityLinkRow')
    if (!securityLinkRow) {
      console.error('[Brave Settings Overrides] Could not find securityLinkRow id on privacy page.')
    } else {
      securityLinkRow.insertAdjacentHTML('afterend',
        `<cr-link-row id="torLinkRow" start-icon="cr:security"
            class="hr" label="${I18nBehavior.i18n('torConnectionSettingsTitle')}"
            sub-label="${I18nBehavior.i18n('torConnectionSettingsDesc')}"
            on-click="onTorClick_"
            role-description="${I18nBehavior.i18n('subpageArrowRoleDescription')}">
         </cr-link-row>`)
    }

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
        pages.insertAdjacentHTML('beforeend', `
          <template is="dom-if" route-path="/privacy/tor">
          <settings-subpage id="tor" page-title="${I18nBehavior.i18n('torConnectionSettingsTitle')}"
              associated-control="[[$$('#torLinkRow')]]"
              learn-more-url="${I18nBehavior.i18n('torConnectionSettingsDesc')}">
            <settings-brave-tor-subpage prefs="{{prefs}}"
            </settings-brave-tor-subpage>
          </settings-subpage>
         </template>`)
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
  },
})
