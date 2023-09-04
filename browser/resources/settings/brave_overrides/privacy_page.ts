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
          <div id="page-title" class="content-settings-header secondary"></div>
          <settings-category-default-radio-group
              id="googleSignInDefault"
              category="[[contentSettingsTypesEnum_.GOOGLE_SIGN_IN]]"
              allow-option-icon="user"
              block-option-icon="user-off">
          </settings-category-default-radio-group>
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
      const pageTitle =
        googleSignInTemplate.content.getElementById('page-title')
      if (!pageTitle) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Google sigin page-title')
      } else {
        let pageTitleNode = document.createTextNode(
          loadTimeData.getString('siteSettingsCategoryGoogleSignIn'))
        pageTitle.appendChild(pageTitleNode)
      }
      const googleSignInDefault =
        googleSignInTemplate.content.getElementById('googleSignInDefault')
      if (!googleSignInDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Google signin default')
      } else {
        googleSignInDefault.setAttribute(
            'block-option-label',
            loadTimeData.getString('siteSettingsGoogleSignInBlock'))
        googleSignInDefault.setAttribute(
            'allow-option-label',
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

function InsertLocalhostAccessSubpage (
  templateContent: DocumentFragment,
  pages: Element)
{
  pages.insertAdjacentHTML(
    'beforeend',
    getTrustedHTML`
        <template is="dom-if" route-path="/content/localhostAccess" no-search>
        <settings-subpage>
          <div id="page-title" class="content-settings-header secondary"></div>
          <settings-category-default-radio-group
              id="localhostAccessDefault"
              category="[[contentSettingsTypesEnum_.LOCALHOST_ACCESS]]"
              allow-option-icon="smartphone-desktop"
              block-option-icon="smartphone-desktop-off">
          </settings-category-default-radio-group>
          <category-setting-exceptions
            id="localhostExceptions"
            category="[[contentSettingsTypesEnum_.LOCALHOST_ACCESS]]">
          </category-setting-exceptions>
        </settings-subpage>
        </template>
      `)

  const localhostAccessTemplate = templateContent.
    querySelector('[route-path="/content/localhostAccess"]')
  if (!localhostAccessTemplate) {
    console.error(
      '[Brave Settings Overrides] Couldn\'t find localhost access template')
  } else {
    const localhostAccessSubpage =
      localhostAccessTemplate.content.querySelector('settings-subpage')
    if (!localhostAccessSubpage) {
      console.error(
        '[Brave Settings Overrides] Couldn\'t find localhost access subpage')
    } else {
      const pageTitle =
        localhostAccessTemplate.content.getElementById('page-title')
      if (!pageTitle) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find localhost access ' +
          'page-title')
      } else {
        let pageTitleNode = document.createTextNode(
          loadTimeData.getString('siteSettingsCategoryLocalhostAccess'))
        pageTitle.appendChild(pageTitleNode)
      }
      const localhostAccessDefault =
        localhostAccessTemplate.content.getElementById('localhostAccessDefault')
      if (!localhostAccessDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find localhost access default')
      } else {
        localhostAccessDefault.setAttribute(
          'block-option-label',
          loadTimeData.getString('siteSettingsLocalhostAccessBlock'))
          localhostAccessDefault.setAttribute(
          'allow-option-label',
          loadTimeData.getString('siteSettingsLocalhostAccessAsk'))
      }
      const localhostExceptions =
        localhostAccessTemplate.content.getElementById('localhostExceptions')
      if (!localhostExceptions) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find localhost exceptions')
      } else {
        localhostExceptions.setAttribute(
          'block-header',
          loadTimeData.getString('siteSettingsLocalhostAccessBlockExceptions'))
        localhostExceptions.setAttribute(
          'allow-header',
          loadTimeData.getString('siteSettingsLocalhostAccessAllowExceptions'))
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
          <div id="page-title" class="content-settings-header secondary"></div>
          <settings-category-default-radio-group
              id="autoplayDefault"
              category="[[contentSettingsTypesEnum_.AUTOPLAY]]"
              allow-option-icon="autoplay-on"
              block-option-icon="autoplay-off">
          </settings-category-default-radio-group>
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
      const pageTitle =
        autoplayTemplate.content.getElementById('page-title')
      if (!pageTitle) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find autoplay page-title')
      } else {
        let pageTitleNode = document.createTextNode(
          loadTimeData.getString('siteSettingsCategoryAutoplay'))
        pageTitle.appendChild(pageTitleNode)
      }
      const autoplayDefault =
        autoplayTemplate.content.getElementById('autoplayDefault')
      if (!autoplayDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find autoplay default')
      } else {
        autoplayDefault.setAttribute(
            'block-option-label',
            loadTimeData.getString('siteSettingsAutoplayBlock'))
        autoplayDefault.setAttribute(
            'allow-option-label',
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
          <div id="page-title" class="content-settings-header secondary"></div>
          <settings-category-default-radio-group
              id="ethereumDefault"
              category="[[contentSettingsTypesEnum_.ETHEREUM]]"
              allow-option-icon="ethereum-on"
              block-option-icon="ethereum-off">
          </settings-category-default-radio-group>
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
      const pageTitle =
        ethereumTemplate.content.getElementById('page-title')
      if (!pageTitle) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Ethereum page-title')
      } else {
        let pageTitleNode = document.createTextNode(
          loadTimeData.getString('siteSettingsCategoryEthereum'))
        pageTitle.appendChild(pageTitleNode)
      }
      const ethereumDefault =
        ethereumTemplate.content.getElementById('ethereumDefault')
      if (!ethereumDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Ethereum default')
      } else {
        ethereumDefault.setAttribute(
            'block-option-label',
            loadTimeData.getString('siteSettingsEthereumBlock'))
        ethereumDefault.setAttribute(
            'allow-option-label',
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
          <div id="page-title" class="content-settings-header secondary"></div>
          <settings-category-default-radio-group
              id="solanaDefault"
              category="[[contentSettingsTypesEnum_.SOLANA]]"
              allow-option-icon="solana-on"
              block-option-icon="solana-off">
          </settings-category-default-radio-group>
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
      const pageTitle =
        solanaTemplate.content.getElementById('page-title')
      if (!pageTitle) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Solana page-title')
      } else {
        let pageTitleNode = document.createTextNode(
          loadTimeData.getString('siteSettingsCategorySolana'))
        pageTitle.appendChild(pageTitleNode)
      }
      const solanaDefault =
        solanaTemplate.content.getElementById('solanaDefault')
      if (!solanaDefault) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Solana default')
      } else {
        solanaDefault.setAttribute(
            'block-option-label',
            loadTimeData.getString('siteSettingsSolanaBlock'))
        solanaDefault.setAttribute(
            'allow-option-label',
            loadTimeData.getString('siteSettingsSolanaAsk'))
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
      const isLocalhostAccessFeatureEnabled =
        loadTimeData.getBoolean('isLocalhostAccessFeatureEnabled')
      if (isLocalhostAccessFeatureEnabled) {
        InsertLocalhostAccessSubpage(templateContent, pages)
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
