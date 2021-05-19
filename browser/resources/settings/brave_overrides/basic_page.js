// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {RegisterStyleOverride,RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'
import {Router} from '../router.js'
import {loadTimeData} from '../i18n_setup.js'
import {pageVisibility} from './page_visibility.js'

import '../getting_started_page/getting_started.js'
import '../brave_default_extensions_page/brave_default_extensions_page.m.js'
import '../default_brave_shields_page/default_brave_shields_page.m.js'
import '../social_blocking_page/social_blocking_page.m.js'
import '../brave_ipfs_page/brave_ipfs_page.m.js'
import '../brave_wallet_page/brave_wallet_page.m.js'
import '../brave_sync_page/brave_sync_page.js'
import '../brave_help_tips_page/brave_help_tips_page.m.js'
import '../brave_new_tab_page/brave_new_tab_page.m.js'

const isGuest = loadTimeData.getBoolean('isGuest')

export function getSectionElement (templateContent, sectionName) {
  const sectionEl = templateContent.querySelector(`template[if*='pageVisibility.${sectionName}']`) ||
    templateContent.querySelector(`settings-section[section="${sectionName}"]`)
  if (!sectionEl) {
    console.error(`[Brave Settings Overrides] Could not find section '${sectionName}'`)
  }
  return sectionEl
}

/**
 * Creates a settings-section element with a single child and returns it.
 * @param {string} sectionName - value of the section attribute
 * @param {string} titleName - loadTimeData key for page-title
 * @param {string} childName - name of child element
 * @param {Object} childAttributes - key-value pairs of child element attributes
 * @returns {Element}
 */
function createSectionElement (sectionName, titleName, childName, childAttributes) {
  const el = document.createElement('settings-section')
  el.setAttribute('page-title', loadTimeData.getString(titleName))
  el.setAttribute('section', sectionName)
  const child = document.createElement(childName)
  for (const attribute in childAttributes) {
    child.setAttribute(attribute, childAttributes[attribute])
  }
  el.appendChild(child)
  return el
}

RegisterStyleOverride(
  'settings-basic-page',
  html`
    <style>
      :host {
        min-width: 544px !important;
      }
    </style>
  `
)

RegisterPolymerTemplateModifications({
  'settings-basic-page': (templateContent) => {
    // Routes
    const r = Router.getInstance().routes_
    if (!r.BASIC) {
      console.error('[Brave Settings Overrides] Routes: could not find BASIC page')
    }
    if (pageVisibility.getStarted) {
      r.GET_STARTED = r.BASIC.createSection('/getStarted', 'getStarted')
      // bring back people's /manageProfile (now in getStarted)
      r.MANAGE_PROFILE = r.GET_STARTED.createChild('/manageProfile');
    }
    r.SHIELDS = r.BASIC.createSection('/shields', 'shields')
    r.SOCIAL_BLOCKING = r.BASIC.createSection('/socialBlocking', 'socialBlocking')
    r.EXTENSIONS = r.BASIC.createSection('/extensions', 'extensions')
    if (pageVisibility.braveSync) {
      r.BRAVE_SYNC = r.BASIC.createSection('/braveSync', 'braveSync')
      r.BRAVE_SYNC_SETUP = r.BRAVE_SYNC.createChild('/braveSync/setup');
    }
    if (pageVisibility.braveIPFS) {
      r.BRAVE_IPFS = r.BASIC.createSection('/ipfs', 'ipfs')
      r.BRAVE_IPFS_KEYS = r.BRAVE_IPFS.createChild('/ipfs/keys');
      r.BRAVE_IPFS_PEERS = r.BRAVE_IPFS.createChild('/ipfs/peers');
    }
    if (pageVisibility.braveWallet) {
      r.BRAVE_WALLET = r.BASIC.createSection('/wallet', 'wallet')
    }

    r.BRAVE_HELP_TIPS = r.BASIC.createSection('/braveHelpTips', 'braveHelpTips')
    r.BRAVE_NEW_TAB = r.BASIC.createSection('/newTab', 'newTab')
    if (r.SITE_SETTINGS) {
      r.SITE_SETTINGS_AUTOPLAY = r.SITE_SETTINGS.createChild('autoplay')
    } else if (!isGuest) {
      console.error('[Brave Settings Overrides] Routes: could not find SITE_SETTINGS page')
    }
    // Autofill route is moved to advanced,
    // otherwise its sections won't show up when opened.
    if (r.AUTOFILL && r.ADVANCED) {
      r.AUTOFILL.parent = r.ADVANCED
    } else if (!isGuest) {
      console.error('[Brave Settings Overrides] Could not move autofill route to advanced route', r)
    }
    // Privacy route is moved to advanced.
    if (r.PRIVACY && r.ADVANCED) {
      r.PRIVACY.parent = r.ADVANCED
      r.CLEAR_BROWSER_DATA.parent = r.ADVANCED
    } else if (!isGuest) {
      console.error('[Brave Settings Overrides] Could not move privacy route to advanced route', r)
    }
    // Add 'Getting Started' section
    // Entire content is wrapped in another conditional template
    const actualTemplate = templateContent.querySelector('template')
    if (!actualTemplate) {
      console.error('[Brave Settings Overrides] Could not find basic-page template')
      return
    }
    const basicPageEl = actualTemplate.content.querySelector('#basicPage')
    if (!basicPageEl) {
      console.error('[Brave Settings Overrides] Could not find basicPage element to insert Getting Started section')
    } else {
      const sectionGetStarted = document.createElement('template')
      sectionGetStarted.setAttribute('is', 'dom-if')
      sectionGetStarted.setAttribute('restamp', true)
      sectionGetStarted.setAttribute('if', '[[showPage_(pageVisibility.getStarted)]]')
      sectionGetStarted.content.appendChild(createSectionElement(
        'getStarted',
        'braveGetStartedTitle',
        'brave-settings-getting-started',
        {
          prefs: '{{prefs}}',
          'page-visibility': '[[pageVisibility]]'
        }
      ))
      const sectionExtensions = document.createElement('template')
      sectionExtensions.setAttribute('is', 'dom-if')
      sectionExtensions.setAttribute('restamp', true)
      sectionExtensions.setAttribute('if', '[[showPage_(pageVisibility.extensions)]]')
      sectionExtensions.content.appendChild(createSectionElement(
        'extensions',
        'braveDefaultExtensions',
        'settings-brave-default-extensions-page',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionIPFS = document.createElement('template')
      sectionIPFS.setAttribute('is', 'dom-if')
      sectionIPFS.setAttribute('restamp', true)
      sectionIPFS.setAttribute('if', '[[showPage_(pageVisibility.braveIPFS)]]')
      sectionIPFS.content.appendChild(createSectionElement(
        'ipfs',
        'braveIPFS',
        'settings-brave-ipfs-page',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionWallet = document.createElement('template')
      sectionWallet.setAttribute('is', 'dom-if')
      sectionWallet.setAttribute('restamp', true)
      sectionWallet.setAttribute('if', '[[showPage_(pageVisibility.braveWallet)]]')
      sectionWallet.content.appendChild(createSectionElement(
        'wallet',
        'braveWallet',
        'settings-brave-wallet-page',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionSync = document.createElement('template')
      sectionSync.setAttribute('is', 'dom-if')
      sectionSync.setAttribute('restamp', true)
      sectionSync.setAttribute('if', '[[showPage_(pageVisibility.braveSync)]]')
      sectionSync.content.appendChild(createSectionElement(
        'braveSync',
        'braveSync',
        'settings-brave-sync-page',
        {}
      ))

      const sectionShields = document.createElement('template')
      sectionShields.setAttribute('is', 'dom-if')
      sectionShields.setAttribute('restamp', true)
      sectionShields.setAttribute('if', '[[showPage_(pageVisibility.shields)]]')
      sectionShields.content.appendChild(createSectionElement(
        'shields',
        'braveShieldsTitle',
        'settings-default-brave-shields-page',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionSocialBlocking = document.createElement('template')
      sectionSocialBlocking.setAttribute('is', 'dom-if')
      sectionSocialBlocking.setAttribute('restamp', true)
      sectionSocialBlocking.setAttribute('if', '[[showPage_(pageVisibility.socialBlocking)]]')
      sectionSocialBlocking.content.appendChild(createSectionElement(
        'socialBlocking',
        'socialBlocking',
        'settings-social-blocking-page',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionHelpTips = document.createElement('template')
      sectionHelpTips.setAttribute('is', 'dom-if')
      sectionHelpTips.setAttribute('restamp', true)
      sectionHelpTips.setAttribute('if', '[[showPage_(pageVisibility.braveHelpTips)]]')
      sectionHelpTips.content.appendChild(createSectionElement(
        'braveHelpTips',
        'braveHelpTips',
        'settings-brave-help-tips-page',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionNewTab = document.createElement('template')
      sectionNewTab.setAttribute('is', 'dom-if')
      sectionNewTab.setAttribute('restamp', true)
      sectionNewTab.setAttribute('if', '[[showPage_(pageVisibility.newTab)]]')
      sectionNewTab.content.appendChild(createSectionElement(
        'newTab',
        'braveNewTab',
        'settings-brave-new-tab-page',
        {
          prefs: '{{prefs}}'
        }
      ))
      // Get Started at top
      basicPageEl.insertAdjacentElement('afterbegin', sectionGetStarted)
      // Move Appearance item
      const sectionAppearance = getSectionElement(actualTemplate.content, 'appearance')
      sectionGetStarted.insertAdjacentElement('afterend', sectionAppearance)
      // Insert New Tab
      sectionAppearance.insertAdjacentElement('afterend', sectionNewTab)
      // Insert sync
      sectionNewTab.insertAdjacentElement('afterend', sectionSync)
      // Insert shields
      sectionSync.insertAdjacentElement('afterend', sectionShields)
      // Insert Social Blocking
      sectionShields.insertAdjacentElement('afterend', sectionSocialBlocking)
      // Move search
      const sectionSearch = getSectionElement(actualTemplate.content, 'search')
      sectionSocialBlocking.insertAdjacentElement('afterend', sectionSearch)
      // Insert extensions
      sectionSearch.insertAdjacentElement('afterend', sectionExtensions)
      // Insert Wallet
      sectionExtensions.insertAdjacentElement('afterend', sectionWallet)
      // Insert IPFS
      sectionWallet.insertAdjacentElement('afterend', sectionIPFS)
      // Advanced
      const advancedTemplate = templateContent.querySelector('template[if="[[showAdvancedSettings_(pageVisibility.advancedSettings)]]"]')
      if (!advancedTemplate) {
        console.error('[Brave Settings Overrides] Could not find advanced section')
      }
      const advancedSubSectionsTemplate = advancedTemplate.content.querySelector('settings-idle-load template')
      if (!advancedSubSectionsTemplate) {
        console.error('[Brave Settings Overrides] Could not find advanced sub-sections container')
      }
      const advancedToggleTemplate = advancedTemplate.content.querySelector('template')
      if (!advancedToggleTemplate) {
        console.error('[Brave Settings Overrides] Could not find advanced toggle template')
      }
      const advancedToggleText = advancedToggleTemplate.content.querySelector('cr-button span')
      if (!advancedToggleText) {
        console.error('[Brave Settings Overrides] Could not find advanced toggle text')
      }
      advancedToggleText.innerText = loadTimeData.getString('braveAdditionalSettingsTitle')
      // Move autofill to before languages
      const sectionAutofill = getSectionElement(actualTemplate.content, 'autofill')
      const sectionLanguages = getSectionElement(advancedSubSectionsTemplate.content, 'languages')
      sectionLanguages.insertAdjacentElement('beforebegin', sectionAutofill)
      // Move privacy to before autofill
      const sectionPrivacy = getSectionElement(actualTemplate.content, 'privacy')
      sectionAutofill.insertAdjacentElement('beforebegin', sectionPrivacy)
      // Move help tips after downloads
      const sectionDownloads = getSectionElement(advancedSubSectionsTemplate.content, 'downloads')
      sectionDownloads.insertAdjacentElement('afterend', sectionHelpTips)
    }
  }
})
