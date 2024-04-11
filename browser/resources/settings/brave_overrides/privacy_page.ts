// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {html, RegisterPolymerTemplateModifications, RegisterPolymerComponentReplacement} from 'chrome://resources/brave/polymer_overriding.js'
import {BraveSettingsPrivacyPageElement} from '../brave_privacy_page/brave_privacy_page.js'
import {loadTimeData} from '../i18n_setup.js'

function InsertGoogleSignInSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.appendChild(
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

function InsertLocalhostAccessSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.appendChild(
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

function InsertAutoplaySubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.appendChild(
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

function InsertEthereumSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.appendChild(
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

function InsertSolanaSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.appendChild(
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

function InsertShieldsSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.appendChild(
    html`
      <template is="dom-if" route-path="/content/braveShields" no-search>
        <settings-subpage
          associated-control="[[$$('#braveShields')]]"
          page-title="${loadTimeData.getString('siteSettingsShieldsStatus')}">
          <category-setting-exceptions
            id="shieldsExceptions"
            category="[[contentSettingsTypesEnum_.BRAVE_SHIELDS]]"
            block-header="${loadTimeData.getString('siteSettingsShieldsDown')}"
            allow-header="${loadTimeData.getString('siteSettingsShieldsUp')}">
          </category-setting-exceptions>
        </settings-subpage>
      </template>
    `)
}

function InsertCookiesSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.appendChild(
    html`
      <template is="dom-if" route-path="/cookies/detail" no-search>
        <settings-subpage
          associated-control="[[$$('#cookiesLink')]]"
          page-title="[[pageTitle]]">
          <cr-button slot="subpage-title-extra" id="remove-all-button"
            on-click="onRemoveAllCookiesFromSite_">
            ${loadTimeData.getString('siteSettingsCookieRemoveAll')}
          </cr-button>
          <brave-site-data-details-subpage page-title="{{pageTitle}}">
          </brave-site-data-details-subpage>
        </settings-subpage>
      </template>
    `)
}

RegisterPolymerComponentReplacement(
  'settings-privacy-page',
  BraveSettingsPrivacyPageElement
)

RegisterPolymerTemplateModifications({
  'settings-privacy-page': (templateContent) => {
    const pages = templateContent.getElementById('pages')
    if (!pages) {
      console.error(
        `[Brave Settings Overrides] Couldn't find privacy_page #pages`)
    } else {
      if (!loadTimeData.getBoolean('isIdleDetectionFeatureEnabled')) {
        const idleDetection = templateContent.querySelector(
          '[route-path="/content/idleDetection"]')
        if (!idleDetection) {
          console.error(
            `[Brave Settings Overrides] Couldn't find idle detection template`)
        } else {
          idleDetection.content.firstElementChild.hidden = true
        }
      }
      const isGoogleSignInFeatureEnabled =
        loadTimeData.getBoolean('isGoogleSignInFeatureEnabled')
      if (isGoogleSignInFeatureEnabled) {
        InsertGoogleSignInSubpage(templateContent, pages)
      }
      const isLocalhostAccessFeatureEnabled =
        loadTimeData.getBoolean('isLocalhostAccessFeatureEnabled')
      if (isLocalhostAccessFeatureEnabled) {
        InsertLocalhostAccessSubpage(templateContent, pages)
      }
      InsertAutoplaySubpage(templateContent, pages)
      const isNativeBraveWalletEnabled =
        loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')
      if (isNativeBraveWalletEnabled) {
        InsertEthereumSubpage(templateContent, pages)
        InsertSolanaSubpage(templateContent, pages)
      }
      InsertShieldsSubpage(templateContent, pages)
      InsertCookiesSubpage(templateContent, pages)
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

    const sotrageAccessTemplate = templateContent.querySelector(
      `template[is=dom-if][route-path='/content/storageAccess'`)
    if (!sotrageAccessTemplate) {
      console.error(
        '[Brave Settings Overrides] Could not find template with' +
        ' route-path=/content/storageAccess on privacy page.')
    } else {
      sotrageAccessTemplate.remove()
    }
  },
})
