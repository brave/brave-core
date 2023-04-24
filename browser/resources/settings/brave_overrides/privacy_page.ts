// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {RegisterPolymerTemplateModifications, RegisterPolymerComponentReplacement} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'
import {BraveSettingsPrivacyPageElement} from '../brave_privacy_page/brave_privacy_page.js'
import {loadTimeData} from '../i18n_setup.js'

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
      googleSignInSubpage.setAttribute(
          'page-title',
          loadTimeData.getString('siteSettingsCategoryGoogleSignIn'))
      const googleSignInDefault =
        googleSignInTemplate.content.getElementById('googleSignInDefault')
      if (!googleSignInDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Google signin default')
      } else {
        googleSignInDefault.setAttribute(
            'toggle-off-label',
            loadTimeData.getString('siteSettingsGoogleSignInAsk'))
        googleSignInDefault.setAttribute(
            'toggle-on-label',
            loadTimeData.getString('siteSettingsGoogleSignInAsk'))
      }
      const googleSignInExceptions =
        googleSignInTemplate.content.getElementById('googleSignInExceptions')
      if (!googleSignInExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Google signin exceptions')
      } else {
        googleSignInExceptions.setAttribute(
            'block-header',
            loadTimeData.getString('siteSettingsGoogleSignInBlockExceptions'))
        googleSignInExceptions.setAttribute(
            'allow-header',
            loadTimeData.getString('siteSettingsGoogleSignInAllowExceptions'))
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
      autoplaySubpage.setAttribute(
          'page-title', loadTimeData.getString('siteSettingsCategoryAutoplay'))
      const autoplayDefault =
        autoplayTemplate.content.getElementById('autoplayDefault')
      if (!autoplayDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find autoplay default')
      } else {
        autoplayDefault.setAttribute(
            'toggle-off-label',
            loadTimeData.getString('siteSettingsAutoplayBlock'))
        autoplayDefault.setAttribute(
            'toggle-on-label',
            loadTimeData.getString('siteSettingsAutoplayAllow'))
      }
      const autoplayExceptions =
        autoplayTemplate.content.getElementById('autoplayExceptions')
      if (!autoplayExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find autoplay exceptions')
      } else {
        autoplayExceptions.setAttribute(
            'block-header', loadTimeData.getString('siteSettingsBlock'))
        autoplayExceptions.setAttribute(
            'allow-header', loadTimeData.getString('siteSettingsAllow'))
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
      ethereumSubpage.setAttribute(
          'page-title', loadTimeData.getString('siteSettingsCategoryEthereum'))
      const ethereumDefault =
        ethereumTemplate.content.getElementById('ethereumDefault')
      if (!ethereumDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Ethereum default')
      } else {
        ethereumDefault.setAttribute(
            'toggle-off-label',
            loadTimeData.getString('siteSettingsEthereumBlock'))
        ethereumDefault.setAttribute(
            'toggle-on-label',
            loadTimeData.getString('siteSettingsEthereumAsk'))
      }
      const ethereumExceptions =
        ethereumTemplate.content.getElementById('ethereumExceptions')
      if (!ethereumExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Ethereum exceptions')
      } else {
        ethereumExceptions.setAttribute(
            'block-header', loadTimeData.getString('siteSettingsBlock'))
        ethereumExceptions.setAttribute(
            'allow-header', loadTimeData.getString('siteSettingsAllow'))
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
      solanaSubpage.setAttribute(
          'page-title', loadTimeData.getString('siteSettingsCategorySolana'))
      const solanaDefault =
        solanaTemplate.content.getElementById('solanaDefault')
      if (!solanaDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Solana default')
      } else {
        solanaDefault.setAttribute(
            'toggle-off-label',
            loadTimeData.getString('siteSettingsSolanaBlock'))
        solanaDefault.setAttribute(
            'toggle-on-label', loadTimeData.getString('siteSettingsSolanaAsk'))
      }
      const solanaExceptions =
        solanaTemplate.content.getElementById('solanaExceptions')
      if (!solanaExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Solana exceptions')
      } else {
        solanaExceptions.setAttribute(
            'block-header', loadTimeData.getString('siteSettingsBlock'))
        solanaExceptions.setAttribute(
            'allow-header', loadTimeData.getString('siteSettingsAllow'))
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
      shieldsSubpage.setAttribute(
          'page-title', loadTimeData.getString('siteSettingsShieldsStatus'))
      const shieldsExceptions =
        shieldsTemplate.content.getElementById('shieldsExceptions')
      if (!shieldsExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Shields exceptions')
      } else {
        shieldsExceptions.setAttribute(
            'block-header', loadTimeData.getString('siteSettingsShieldsDown'))
        shieldsExceptions.setAttribute(
            'allow-header', loadTimeData.getString('siteSettingsShieldsUp'))
      }
    }
  }
}

function InsertCookiesSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.insertAdjacentHTML(
    'beforeend',
    getTrustedHTML`
      <template is="dom-if" route-path="/cookies/detail" no-search>
        <settings-subpage page-title="[[pageTitle]]">
          <cr-button slot="subpage-title-extra" id="remove-all-button"
            on-click="onRemoveAllCookiesFromSite_">
          </cr-button>
          <brave-site-data-details-subpage page-title="{{pageTitle}}">
          </brave-site-data-details-subpage>
        </settings-subpage>
      </template>`)
  const cookiesTemplate = templateContent.
      querySelector('[route-path="/cookies/detail"]')
  if (!cookiesTemplate) {
    console.error(
      '[Brave Settings Overrides] Couldn\'t find Cookies template')
  } else {
    const cookiesSubpage =
      cookiesTemplate.content.querySelector('settings-subpage')
    if (!cookiesSubpage) {
      console.error(
        '[Brave Settings Overrides] Couldn\'t find Cookies subpage')
    } else {
      const removeAllButton =
        cookiesTemplate.content.getElementById('remove-all-button')
      if (!removeAllButton) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Cookies remove all button')
      } else {
        removeAllButton.textContent =
          loadTimeData.getString('siteSettingsCookieRemoveAll');
      }
    }
  }
}

RegisterPolymerComponentReplacement(
  'settings-privacy-page',
  BraveSettingsPrivacyPageElement
)

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

    const showPrivacyGuideEntryPointTemplate =
        templateContent.querySelector(`template[if*='isPrivacyGuideAvailable']`)
    if (!showPrivacyGuideEntryPointTemplate) {
      console.error(
          '[Brave Settings Overrides] Could not find template with if*=isPrivacyGuideAvailable on privacy page.')
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
