/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {html, RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'
import {loadTimeData} from '../i18n_setup.js'

function InsertGoogleSignInSubpage (section: Element)
{
  section.appendChild(
    html`
      <template is="dom-if" route-path="/content/googleSignIn" no-search>
        <settings-subpage
          associated-control="[[$$('#googleSignIn')]]"
          page-title="${loadTimeData.getString('siteSettingsGoogleSignIn')}">
          <settings-category-default-radio-group
              id="googleSignInDefault"
              category="[[contentSettingsTypesEnum_.GOOGLE_SIGN_IN]]"
              block-option-label=
                "${loadTimeData.getString('siteSettingsGoogleSignInBlock')}"
              allow-option-label=
                "${loadTimeData.getString('siteSettingsGoogleSignInAsk')}"
              allow-option-icon="user"
              block-option-icon="user-off">
          </settings-category-default-radio-group>
          <category-setting-exceptions
            id="googleSignInExceptions"
            category="[[contentSettingsTypesEnum_.GOOGLE_SIGN_IN]]"
            block-header=
              "${loadTimeData.getString(
                'siteSettingsGoogleSignInBlockExceptions')}"
            allow-header=
              "${loadTimeData.getString(
                'siteSettingsGoogleSignInAllowExceptions')}">
          </category-setting-exceptions>
        </settings-subpage>
      </template>
    `)
}

function InsertLocalhostAccessSubpage (section: Element)
{
  section.appendChild(
    html`
      <template is="dom-if" route-path="/content/localhostAccess" no-search>
        <settings-subpage
          associated-control="[[$$('#localhostAccess')]]"
          page-title="${loadTimeData.getString('siteSettingsLocalhostAccess')}">
          <settings-category-default-radio-group
              id="localhostAccessDefault"
              category="[[contentSettingsTypesEnum_.LOCALHOST_ACCESS]]"
              block-option-label=
                "${loadTimeData.getString('siteSettingsLocalhostAccessBlock')}"
              allow-option-label=
                "${loadTimeData.getString('siteSettingsLocalhostAccessAsk')}"
              allow-option-icon="smartphone-desktop"
              block-option-icon="smartphone-desktop-off">
          </settings-category-default-radio-group>
          <category-setting-exceptions
            id="localhostExceptions"
            category="[[contentSettingsTypesEnum_.LOCALHOST_ACCESS]]"
            block-header=
              "${loadTimeData.getString(
                'siteSettingsLocalhostAccessBlockExceptions')}"
            allow-header=
              "${loadTimeData.getString(
                'siteSettingsLocalhostAccessAllowExceptions')}">
          </category-setting-exceptions>
        </settings-subpage>
      </template>
    `)
}

function InsertAutoplaySubpage (section: Element)
{
  section.appendChild(
    html`
      <template is="dom-if" route-path="/content/autoplay" no-search>
        <settings-subpage
          associated-control="[[$$('#autoplay')]]"
          page-title="${loadTimeData.getString('siteSettingsAutoplay')}">
          <settings-category-default-radio-group
              id="autoplayDefault"
              category="[[contentSettingsTypesEnum_.AUTOPLAY]]"
              block-option-label=
                "${loadTimeData.getString('siteSettingsAutoplayBlock')}"
              allow-option-label=
                "${loadTimeData.getString('siteSettingsAutoplayAllow')}"
              allow-option-icon="autoplay-on"
              block-option-icon="autoplay-off">
          </settings-category-default-radio-group>
          <category-setting-exceptions
            id="autoplayExceptions"
            category="[[contentSettingsTypesEnum_.AUTOPLAY]]"
            block-header="${loadTimeData.getString('siteSettingsBlock')}"
            allow-header="${loadTimeData.getString('siteSettingsAllow')}">
          </category-setting-exceptions>
        </settings-subpage>
      </template>
    `)
}

function InsertEthereumSubpage (section: Element)
{
  section.appendChild(
    html`
      <template is="dom-if" route-path="/content/ethereum" no-search>
        <settings-subpage
          associated-control="[[$$('#ethereum')]]"
          page-title="${loadTimeData.getString('siteSettingsEthereum')}">
          <settings-category-default-radio-group
              id="ethereumDefault"
              category="[[contentSettingsTypesEnum_.ETHEREUM]]"
              block-option-label=
                "${loadTimeData.getString('siteSettingsEthereumBlock')}"
              allow-option-label=
                "${loadTimeData.getString('siteSettingsEthereumAsk')}"
              allow-option-icon="ethereum-on"
              block-option-icon="ethereum-off">
          </settings-category-default-radio-group>
          <category-setting-exceptions
            id="ethereumExceptions"
            category="[[contentSettingsTypesEnum_.ETHEREUM]]"
            block-header="${loadTimeData.getString('siteSettingsBlock')}"
            allow-header="${loadTimeData.getString('siteSettingsAllow')}"
            read-only-list>
          </category-setting-exceptions>
        </settings-subpage>
      </template>
    `)
}

function InsertSolanaSubpage (section: Element)
{
  section.appendChild(
    html`
      <template is="dom-if" route-path="/content/solana" no-search>
        <settings-subpage
          associated-control="[[$$('#solana')]]"
          page-title="${loadTimeData.getString('siteSettingsSolana')}">
          <settings-category-default-radio-group
              id="solanaDefault"
              category="[[contentSettingsTypesEnum_.SOLANA]]"
              block-option-label=
                "${loadTimeData.getString('siteSettingsSolanaBlock')}"
              allow-option-label=
                "${loadTimeData.getString('siteSettingsSolanaAsk')}"
              allow-option-icon="solana-on"
              block-option-icon="solana-off">
          </settings-category-default-radio-group>
          <category-setting-exceptions
            id="solanaExceptions"
            category="[[contentSettingsTypesEnum_.SOLANA]]"
            block-header="${loadTimeData.getString('siteSettingsBlock')}"
            allow-header="${loadTimeData.getString('siteSettingsAllow')}"
            read-only-list>
          </category-setting-exceptions>
        </settings-subpage>
      </template>
    `)
}

function InsertCardanoSubpage (section: Element)
{
  section.appendChild(
    html`
      <template is="dom-if" route-path="/content/cardano" no-search>
        <settings-subpage
          associated-control="[[$$('#cardano')]]"
          page-title="${loadTimeData.getString('siteSettingsCardano')}">
          <settings-category-default-radio-group
              id="cardanoDefault"
              category="[[contentSettingsTypesEnum_.CARDANO]]"
              block-option-label=
                "${loadTimeData.getString('siteSettingsCardanoBlock')}"
              allow-option-label=
                "${loadTimeData.getString('siteSettingsCardanoAsk')}"
              allow-option-icon="cardano-on"
              block-option-icon="cardano-off">
          </settings-category-default-radio-group>
          <category-setting-exceptions
            id="cardanoExceptions"
            category="[[contentSettingsTypesEnum_.CARDANO]]"
            block-header="${loadTimeData.getString('siteSettingsBlock')}"
            allow-header="${loadTimeData.getString('siteSettingsAllow')}"
            read-only-list>
          </category-setting-exceptions>
        </settings-subpage>
      </template>
    `)
}

function InsertBraveOpenAIChatSubpage (section: Element)
{
  section.appendChild(
    html`
      <template is="dom-if" route-path="/content/braveOpenAIChat" no-search>
        <settings-subpage
          associated-control="[[$$('#braveAIChat')]]"
          page-title="${loadTimeData.getString('siteSettingsBraveOpenAIChat')}">
          <settings-category-default-radio-group
              id="braveAIChatDefault"
              category="[[contentSettingsTypesEnum_.BRAVE_OPEN_AI_CHAT]]"
              block-option-label=
                "${loadTimeData.getString('siteSettingsBraveOpenAIChatBlock')}"
              allow-option-label=
                "${loadTimeData.getString('siteSettingsBraveOpenAIChatAsk')}"
              allow-option-icon="user"
              block-option-icon="user-off">
          </settings-category-default-radio-group>
          <category-setting-exceptions
            id="braveAIChatExceptions"
            category="[[contentSettingsTypesEnum_.BRAVE_OPEN_AI_CHAT]]"
            block-header="${loadTimeData.getString('siteSettingsBlock')}"
            allow-header="${loadTimeData.getString('siteSettingsAllow')}"
            read-only-list>
          </category-setting-exceptions>
        </settings-subpage>
      </template>
    `)
}

RegisterPolymerTemplateModifications({
  'settings-privacy-page': (templateContent) => {
    const section = templateContent.querySelector('settings-section')
    if (!section) {
      console.error(
        `[Settings] Couldn't find privacy_page settings-section`)
      return
    }
    const isGoogleSignInFeatureEnabled =
      loadTimeData.getBoolean('isGoogleSignInFeatureEnabled')
    if (isGoogleSignInFeatureEnabled) {
      InsertGoogleSignInSubpage(section)
    }
    const isLocalhostAccessFeatureEnabled =
      loadTimeData.getBoolean('isLocalhostAccessFeatureEnabled')
    if (isLocalhostAccessFeatureEnabled) {
      InsertLocalhostAccessSubpage(section)
    }
    const isOpenAIChatFromBraveSearchEnabled =
      loadTimeData.getBoolean('isOpenAIChatFromBraveSearchEnabled')
    if (isOpenAIChatFromBraveSearchEnabled) {
      InsertBraveOpenAIChatSubpage(section)
    }
    InsertAutoplaySubpage(section)
    const isNativeBraveWalletEnabled =
      loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')
    const isCardanoDappSupportFeatureEnabled =
      loadTimeData.getBoolean('isCardanoDappSupportFeatureEnabled')
    const isBraveWalletAllowed =
      loadTimeData.getBoolean('isBraveWalletAllowed')
    if (isNativeBraveWalletEnabled && isBraveWalletAllowed) {
      InsertEthereumSubpage(section)
      InsertSolanaSubpage(section)
      if (isCardanoDappSupportFeatureEnabled) {
        InsertCardanoSubpage(section)
      }
    }
    const permissionsLinkRow =
      templateContent.getElementById('permissionsLinkRow')
    if (!permissionsLinkRow) {
      console.error(
        '[Brave Settings Overrides] Couldn\'t find permissionsLinkRow')
    } else {
      permissionsLinkRow.insertAdjacentHTML(
        'afterend',
        getTrustedHTML`
          <settings-brave-personalization-options prefs="{{prefs}}">
          </settings-brave-personalization-options>
        `)
    }
    const thirdPartyCookiesLinkRow =
      templateContent.getElementById('thirdPartyCookiesLinkRow')
    if (!thirdPartyCookiesLinkRow) {
      console.error(
        '[Brave Settings Overrides] Could not find ' +
        'thirdPartyCookiesLinkRow id on privacy page.')
    } else {
      thirdPartyCookiesLinkRow.setAttribute('hidden', 'true')
    }

    if (!loadTimeData.getBoolean('isPrivacySandboxRestricted')) {
      const privacySandboxSettings3Template = templateContent.
        querySelector(`template[if*='isPrivacySandboxSettings3Enabled_']`)
      if (!privacySandboxSettings3Template) {
        console.error(
          '[Brave Settings Overrides] Could not find template with ' +
          'if*=isPrivacySandboxSettings3Enabled_ on privacy page.')
      } else {
        const privacySandboxLinkRow = privacySandboxSettings3Template.content.
          getElementById('privacySandboxLinkRow')
        if (!privacySandboxLinkRow) {
          console.error(
            '[Brave Settings Overrides] Could not find privacySandboxLinkRow' +
            ' id on privacy page.')
        } else {
          privacySandboxLinkRow.setAttribute('hidden', 'true')
        }
        const privacySandboxLink = privacySandboxSettings3Template.content.
          getElementById('privacySandboxLink')
        if (!privacySandboxLink) {
          console.error(
            '[Brave Settings Overrides] Could not find privacySandboxLink id' +
            ' on privacy page.')
        } else {
          privacySandboxSettings3Template.setAttribute('hidden', 'true')
        }
      }
      const privacySandboxSettings4Template = templateContent.
        querySelector(`template[if*='isPrivacySandboxSettings4Enabled_']`)
      if (!privacySandboxSettings4Template) {
        console.error(
          '[Brave Settings Overrides] Could not find template with ' +
          'if*=isPrivacySandboxSettings4Enabled_ on privacy page.')
      } else {
        const privacySandboxLinkRow = privacySandboxSettings4Template.content.
          getElementById('privacySandboxLinkRow')
        if (!privacySandboxLinkRow) {
          console.error(
            '[Brave Settings Overrides] Could not find privacySandboxLinkRow ' +
            'id on privacy page.')
        } else {
          privacySandboxLinkRow.setAttribute('hidden', 'true')
        }
      }
    }

    const showPrivacyGuideEntryPointTemplate =
      templateContent.querySelector(`template[if*='isPrivacyGuideAvailable']`)
    if (!showPrivacyGuideEntryPointTemplate) {
      console.error(
        '[Brave Settings Overrides] Could not find template with' +
        ' if*=isPrivacyGuideAvailable on privacy page.')
    } else {
      const privacyGuideLinkRow = showPrivacyGuideEntryPointTemplate.content.
        getElementById('privacyGuideLinkRow')
      if (!privacyGuideLinkRow) {
        console.error(
          '[Brave Settings Overrides] Could not find privacyGuideLinkRow id' +
          ' on privacy page.')
      } else {
        privacyGuideLinkRow.setAttribute('hidden', 'true')
      }
    }
  }
})
