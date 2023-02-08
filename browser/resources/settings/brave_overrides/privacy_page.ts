// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {RegisterPolymerTemplateModifications} from 'chrome://resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/i18n_behavior.js'
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'

function InsertGoogleSignInSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.insertAdjacentHTML(
    'beforeend',
    getTrustedHTML`
      <template is="dom-if" route-path="/content/googleSignIn" no-search>
        <settings-subpage>
          <category-default-setting
            id="googleSignInDefault"
            category="[[contentSettingsTypesEnum_.GOOGLE_SIGN_IN]]">
          </category-default-setting>
          <category-setting-exceptions
            id="googleSignInExceptions"
            category="[[contentSettingsTypesEnum_.GOOGLE_SIGN_IN]]">
          </category-setting-exceptions>
        </settings-subpage>
      </template>
    `)
  const googleSignInTemplate = templateContent.
    querySelector('[route-path="/content/googleSignIn"]')
  if (!googleSignInTemplate) {
    console.error(
      '[Brave Settings Overrides] Couldn\'t find Google signin template')
  } else {
    const googleSignInSubpage =
      googleSignInTemplate.content.querySelector('settings-subpage')
    if (!googleSignInSubpage) {
      console.error(
        '[Brave Settings Overrides] Couldn\'t find Google signin subpage')
    } else {
      googleSignInSubpage.setAttribute('page-title',
        I18nBehavior.i18n('siteSettingsCategoryGoogleSignIn'))
      const googleSignInDefault =
        googleSignInTemplate.content.getElementById('googleSignInDefault')
      if (!googleSignInDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Google signin default')
      } else {
        googleSignInDefault.setAttribute(
          'toggle-off-label',
          I18nBehavior.i18n('siteSettingsGoogleSignInAsk'))
        googleSignInDefault.setAttribute(
          'toggle-on-label',
          I18nBehavior.i18n('siteSettingsGoogleSignInAsk'))
      }
      const googleSignInExceptions =
        googleSignInTemplate.content.getElementById('googleSignInExceptions')
      if (!googleSignInExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Google signin exceptions')
      } else {
        googleSignInExceptions.setAttribute(
          'block-header',
          I18nBehavior.i18n('siteSettingsGoogleSignInBlockExceptions'))
        googleSignInExceptions.setAttribute(
          'allow-header',
          I18nBehavior.i18n('siteSettingsGoogleSignInAllowExceptions'))
      }
    }
  }
}

function InsertAutoplaySubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.insertAdjacentHTML(
    'beforeend',
    getTrustedHTML`
      <template is="dom-if" route-path="/content/autoplay" no-search>
        <settings-subpage>
          <category-default-setting
            id="autoplayDefault"
            category="[[contentSettingsTypesEnum_.AUTOPLAY}}">
          </category-default-setting>
          <category-setting-exceptions
            id="autoplayExceptions"
            category="[[contentSettingsTypesEnum_.AUTOPLAY]]">
          </category-setting-exceptions>
        </settings-subpage>
      </template>`)
  const autoplayTemplate = templateContent.
    querySelector('[route-path="/content/autoplay"]')
  if (!autoplayTemplate) {
    console.error(
      '[Brave Settings Overrides] Couldn\'t find autoplay template')
  } else {
    const autoplaySubpage =
      autoplayTemplate.content.querySelector('settings-subpage')
    if (!autoplaySubpage) {
      console.error(
        '[Brave Settings Overrides] Couldn\'t find autoplay subpage')
    } else {
      autoplaySubpage.setAttribute('page-title',
        I18nBehavior.i18n('siteSettingsCategoryAutoplay'))
      const autoplayDefault =
        autoplayTemplate.content.getElementById('autoplayDefault')
      if (!autoplayDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find autoplay default')
      } else {
        autoplayDefault.setAttribute(
          'toggle-off-label',
          I18nBehavior.i18n('siteSettingsAutoplayBlock'))
        autoplayDefault.setAttribute(
          'toggle-on-label',
          I18nBehavior.i18n('siteSettingsAutoplayAllow'))
      }
      const autoplayExceptions =
        autoplayTemplate.content.getElementById('autoplayExceptions')
      if (!autoplayExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find autoplay exceptions')
      } else {
        autoplayExceptions.setAttribute(
          'block-header',
          I18nBehavior.i18n('siteSettingsBlock'))
        autoplayExceptions.setAttribute(
          'allow-header',
          I18nBehavior.i18n('siteSettingsAllow'))
      }
    }
  }
}

function InsertEthereumSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.insertAdjacentHTML(
    'beforeend',
    getTrustedHTML`
      <template is="dom-if" route-path="/content/ethereum" no-search>
        <settings-subpage>
          <category-default-setting
            id="ethereumDefault"
            category="[[contentSettingsTypesEnum_.ETHEREUM]]">
          </category-default-setting>
          <category-setting-exceptions
            id="ethereumExceptions"
            category="[[contentSettingsTypesEnum_.ETHEREUM]]"
            read-only-list>
          </category-setting-exceptions>
        </settings-subpage>
      </template>`)
  const ethereumTemplate = templateContent.
    querySelector('[route-path="/content/ethereum"]')
  if (!ethereumTemplate) {
    console.error(
      '[Brave Settings Overrides] Couldn\'t find Ethereum template')
  } else {
    const ethereumSubpage =
      ethereumTemplate.content.querySelector('settings-subpage')
    if (!ethereumSubpage) {
      console.error(
        '[Brave Settings Overrides] Couldn\'t find Ethereum subpage')
    } else {
      ethereumSubpage.setAttribute('page-title',
        I18nBehavior.i18n('siteSettingsCategoryEthereum'))
      const ethereumDefault =
        ethereumTemplate.content.getElementById('ethereumDefault')
      if (!ethereumDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Ethereum default')
      } else {
        ethereumDefault.setAttribute(
          'toggle-off-label',
          I18nBehavior.i18n('siteSettingsEthereumBlock'))
        ethereumDefault.setAttribute(
          'toggle-on-label',
          I18nBehavior.i18n('siteSettingsEthereumAsk'))
      }
      const ethereumExceptions =
        ethereumTemplate.content.getElementById('ethereumExceptions')
      if (!ethereumExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Ethereum exceptions')
      } else {
        ethereumExceptions.setAttribute(
          'block-header',
          I18nBehavior.i18n('siteSettingsBlock'))
        ethereumExceptions.setAttribute(
          'allow-header',
          I18nBehavior.i18n('siteSettingsAllow'))
      }
    }
  }
}

function InsertSolanaSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.insertAdjacentHTML(
    'beforeend',
    getTrustedHTML`
      <template is="dom-if" route-path="/content/solana" no-search>
        <settings-subpage>
          <category-default-setting
            id="solanaDefault"
            category="[[contentSettingsTypesEnum_.SOLANA]]">
          </category-default-setting>
          <category-setting-exceptions
            id="solanaExceptions"
            category="[[contentSettingsTypesEnum_.SOLANA]]"
            read-only-list>
          </category-setting-exceptions>
        </settings-subpage>
      </template>`)
  const solanaTemplate = templateContent.
    querySelector('[route-path="/content/solana"]')
  if (!solanaTemplate) {
    console.error(
      '[Brave Settings Overrides] Couldn\'t find Solana template')
  } else {
    const solanaSubpage =
      solanaTemplate.content.querySelector('settings-subpage')
    if (!solanaSubpage) {
      console.error(
        '[Brave Settings Overrides] Couldn\'t find Solana subpage')
    } else {
      solanaSubpage.setAttribute('page-title',
        I18nBehavior.i18n('siteSettingsCategorySolana'))
      const solanaDefault =
        solanaTemplate.content.getElementById('solanaDefault')
      if (!solanaDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Solana default')
      } else {
        solanaDefault.setAttribute(
          'toggle-off-label',
          I18nBehavior.i18n('siteSettingsSolanaBlock'))
        solanaDefault.setAttribute(
          'toggle-on-label',
          I18nBehavior.i18n('siteSettingsSolanaAsk'))
      }
      const solanaExceptions =
        solanaTemplate.content.getElementById('solanaExceptions')
      if (!solanaExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Solana exceptions')
      } else {
        solanaExceptions.setAttribute(
          'block-header',
          I18nBehavior.i18n('siteSettingsBlock'))
        solanaExceptions.setAttribute(
          'allow-header',
          I18nBehavior.i18n('siteSettingsAllow'))
      }
    }
  }
}

function InsertShieldsSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.insertAdjacentHTML(
    'beforeend',
    getTrustedHTML`
      <template is="dom-if" route-path="/content/braveShields" no-search>
        <settings-subpage>
          <category-setting-exceptions
            id="shieldsExceptions"
            category="[[contentSettingsTypesEnum_.BRAVE_SHIELDS]]">
          </category-setting-exceptions>
        </settings-subpage>
      </template>`)
  const shieldsTemplate = templateContent.
    querySelector('[route-path="/content/braveShields"]')
  if (!shieldsTemplate) {
    console.error(
      '[Brave Settings Overrides] Couldn\'t find Shields template')
  } else {
    const shieldsSubpage =
      shieldsTemplate.content.querySelector('settings-subpage')
    if (!shieldsSubpage) {
      console.error(
        '[Brave Settings Overrides] Couldn\'t find Shields subpage')
    } else {
      shieldsSubpage.setAttribute('page-title',
        I18nBehavior.i18n('siteSettingsShieldsStatus'))
      const shieldsExceptions =
        shieldsTemplate.content.getElementById('shieldsExceptions')
      if (!shieldsExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Shields exceptions')
      } else {
        shieldsExceptions.setAttribute(
          'block-header',
          I18nBehavior.i18n('siteSettingsShieldsDown'))
        shieldsExceptions.setAttribute(
          'allow-header',
          I18nBehavior.i18n('siteSettingsShieldsUp'))
      }
    }
  }
}

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
      const isGoogleSignInFeatureEnabled =
        loadTimeData.getBoolean('isGoogleSignInFeatureEnabled')
      if (isGoogleSignInFeatureEnabled) {
        InsertGoogleSignInSubpage(templateContent, pages)
      }
      InsertAutoplaySubpage(templateContent, pages)
      const isNativeBraveWalletEnabled = loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')
      if (isNativeBraveWalletEnabled) {
        InsertEthereumSubpage(templateContent, pages)
        InsertSolanaSubpage(templateContent, pages)
      }
      InsertShieldsSubpage(templateContent, pages)
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
          console.error('[Brave Settings Overrides] Could not find privacySandboxLinkRow id on privacy page.')
        } else {
          privacySandboxLinkRow.setAttribute('hidden', 'true')
        }
        const privacySandboxLink = privacySandboxSettings3Template.content.
          getElementById('privacySandboxLink')
        if (!privacySandboxLink) {
          console.error('[Brave Settings Overrides] Could not find privacySandboxLink id on privacy page.')
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
