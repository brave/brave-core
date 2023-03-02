// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import '../brave_default_extensions_page/brave_default_extensions_page.js'
import '../brave_help_tips_page/brave_help_tips_page.js'
import '../brave_ipfs_page/brave_ipfs_page.js'
import '../brave_new_tab_page/brave_new_tab_page.js'
import '../brave_rewards_page/brave_rewards_page.js'
import '../brave_search_engines_page/brave_search_engines_page.js'
import '../brave_sync_page/brave_sync_page.js'
import '../brave_site_settings/brave_site_data_details_subpage.js'
import '../brave_tor_page/brave_tor_subpage.js'
import '../brave_wallet_page/brave_wallet_page.js'
import '../brave_web3_domains_page/brave_web3_domains_page.js'
import '../default_brave_shields_page/default_brave_shields_page.js'
import '../getting_started_page/getting_started.js'
import '../social_blocking_page/social_blocking_page.js'
import '../shortcuts_page/shortcuts_page.js'

import {html, RegisterPolymerTemplateModifications, RegisterStyleOverride} from 'chrome://resources/brave/polymer_overriding.js'

import {loadTimeData} from '../i18n_setup.js'

const isGuest = loadTimeData.getBoolean('isGuest')

export function getSectionElement (templateContent, sectionName) {
  const sectionEl = templateContent.querySelector(`template[if*='showPage_(pageVisibility.${sectionName}']`) ||
    templateContent.querySelector(`template[if*='pageVisibility.${sectionName}']`) ||
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
  const childAttributesString = Object.keys(childAttributes).map(attribute =>
      `${attribute}="${childAttributes[attribute]}"`)
    .join(' ')
  // This needs to be inside a template so that our components do not get created immediately.
  // Otherwise the polymer bindings won't be setup correctly at first.
  return html`
    <settings-section page-title="${loadTimeData.getString(titleName)}" section="${sectionName}">
      <${childName}
        ${childAttributesString}
      >
      </${childName}>
    </settings-section>
  `
}

function createNestedSectionElement(sectionName, nestedUnder, titleName, childName, childAttributes) {
  const childAttributesString = Object.keys(childAttributes).map(attribute =>
    `${attribute}="${childAttributes[attribute]}"`)
    .join(' ')
  // This needs to be inside a template so that our components do not get created immediately.
  // Otherwise the polymer bindings won't be setup correctly at first.
  return html`
    <settings-section page-title="${loadTimeData.getString(titleName)}" section="${sectionName}" nest-under-section="${nestedUnder}">
      <${childName}
        ${childAttributesString}
      >
      </${childName}>
    </settings-section>
  `
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
      const privacyGuidePromoSection = actualTemplate.content.querySelector('#privacyGuidePromoSection')
      if (!privacyGuidePromoSection) {
        console.error('[Brave Settings Overrides] Could not find privacyGuidePromoSection element to hide')
      } else {
        privacyGuidePromoSection.remove()
      }
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
      sectionIPFS.content.appendChild(createNestedSectionElement(
        'ipfs',
        'web3',
        'braveIPFS',
        'settings-brave-ipfs-page',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionTor = document.createElement('template')
      sectionTor.setAttribute('is', 'dom-if')
      sectionTor.setAttribute('restamp', true)
      sectionTor.setAttribute('if', '[[showPage_(pageVisibility.braveTor)]]')
      sectionTor.content.appendChild(createNestedSectionElement(
        'tor',
        'privacy',
        'braveTor',
        'settings-brave-tor-subpage',
        {
          prefs: '{{prefs}}'
        }
      ))
      const isBraveWalletAllowed = loadTimeData.getBoolean('isBraveWalletAllowed')
      let sectionWallet = undefined
      if (isBraveWalletAllowed) {
        sectionWallet = document.createElement('template')
        sectionWallet.setAttribute('is', 'dom-if')
        sectionWallet.setAttribute('restamp', true)
        sectionWallet.setAttribute('if', '[[showPage_(pageVisibility.braveWallet)]]')
        sectionWallet.content.appendChild(createNestedSectionElement(
          'wallet',
          'web3',
          'braveWallet',
          'settings-brave-wallet-page',
          {
            prefs: '{{prefs}}'
          }
        ))
      }
      const sectionWeb3Domains = document.createElement('template')
      sectionWeb3Domains.setAttribute('is', 'dom-if')
      sectionWeb3Domains.setAttribute('restamp', true)
      sectionWeb3Domains.setAttribute('if',
        '[[showPage_(pageVisibility.braveWeb3Domains)]]')
      sectionWeb3Domains.content.appendChild(createNestedSectionElement(
        'web3Domains',
        'web3',
        'braveWeb3Domains',
        'settings-brave-web3-domains-page',
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
      const isBraveRewardsSupported = loadTimeData.getBoolean('isBraveRewardsSupported')
      let sectionRewards = undefined
      if (isBraveRewardsSupported) {
        sectionRewards = document.createElement('template')
        sectionRewards.setAttribute('is', 'dom-if')
        sectionRewards.setAttribute('restamp', true)
        sectionRewards.setAttribute('if', '[[showPage_(pageVisibility.rewards)]]')
        sectionRewards.content.appendChild(createSectionElement(
          'rewards',
          'braveRewards',
          'settings-brave-rewards-page',
          {
            prefs: '{{prefs}}'
          }
        ))
      }
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

      // Remove all hidden performance options from basic page.
      // We moved performance elements in system settings.
      const performanceTemplate = actualTemplate.content.querySelector(
        'template[if="[[showPerformancePage_(pageVisibility.performance)]]"]')
      if (performanceTemplate) {
        performanceTemplate.remove()
      }

      const batteryTemplate = actualTemplate.content.querySelector(
        'template[if="[[showBatteryPage_(pageVisibility.performance)]]"]')
      if (batteryTemplate) {
        batteryTemplate.remove()
      }

      // Get Started at top
      let last = basicPageEl.insertAdjacentElement('afterbegin',
        sectionGetStarted)
      // Move Appearance item
      const sectionAppearance = getSectionElement(actualTemplate.content,
        'appearance')
      last = last.insertAdjacentElement('afterend', sectionAppearance)
      // Insert New Tab
      last = last.insertAdjacentElement('afterend', sectionNewTab)
      // Insert shields
      last = last.insertAdjacentElement('afterend', sectionShields)
      // Insert Rewards
      if (sectionRewards) {
        last = last.insertAdjacentElement('afterend', sectionRewards)
      }
      // Insert Social Blocking
      last = last.insertAdjacentElement('afterend', sectionSocialBlocking)
      // Move privacy section to after social blocking
      const sectionPrivacy = getSectionElement(actualTemplate.content, 'privacy')
      last = last.insertAdjacentElement('afterend', sectionPrivacy)
      // Insert sync
      last = last.insertAdjacentElement('afterend', sectionSync)
      // Move search
      const sectionSearch = getSectionElement(actualTemplate.content, 'search')
      last = last.insertAdjacentElement('afterend', sectionSearch)
      // Insert extensions
      last = last.insertAdjacentElement('afterend', sectionExtensions)
      // Insert Wallet
      if (sectionWallet) {
        last = last.insertAdjacentElement('afterend', sectionWallet)
      }
      // Insert IPFS
      last = last.insertAdjacentElement('afterend', sectionIPFS)
      // Insert Web3 Domains
      last = last.insertAdjacentElement('afterend', sectionWeb3Domains)
      // Insert Tor
      last = last.insertAdjacentElement('afterend', sectionTor)

      // Advanced
      const advancedTemplate = templateContent.querySelector('template[if="[[showAdvancedSettings_(pageVisibility.advancedSettings)]]"]')
      if (!advancedTemplate) {
        console.error('[Brave Settings Overrides] Could not find advanced section')
      }
      const advancedSubSectionsTemplate = advancedTemplate.content.querySelector('settings-idle-load template')
      if (!advancedSubSectionsTemplate) {
        console.error('[Brave Settings Overrides] Could not find advanced sub-sections container')
      }
      // Move autofill to before languages
      const sectionAutofill = getSectionElement(actualTemplate.content, 'autofill')
      const sectionLanguages = getSectionElement(advancedSubSectionsTemplate.content, 'languages')
      sectionLanguages.insertAdjacentElement('beforebegin', sectionAutofill)
      // Move help tips after downloads
      const sectionDownloads = getSectionElement(advancedSubSectionsTemplate.content, 'downloads')
      sectionDownloads.insertAdjacentElement('afterend', sectionHelpTips)
    }
  }
})
