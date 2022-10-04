// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import { html } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { RegisterStyleOverride, RegisterPolymerTemplateModifications } from 'chrome://resources/polymer_overriding.js'
import { loadTimeData } from '../i18n_setup.js'
import '../brave_icons.html.js'

function createMenuElement(title, href, iconName, pageVisibilitySection) {
  const menuEl = document.createElement('a')
  if (pageVisibilitySection) {
    menuEl.setAttribute('hidden', `[[!pageVisibility.${pageVisibilitySection}]]`)
  }
  menuEl.href = href
  menuEl.setAttribute('role', 'menuitem')
  menuEl.setAttribute('class', 'cr-nav-menu-item')
  const iconChild = document.createElement('iron-icon')
  iconChild.setAttribute('icon', iconName)
  menuEl.appendChild(iconChild)
  const text = document.createTextNode(title)
  menuEl.appendChild(text)
  const paperRippleChild = document.createElement('paper-ripple')
  menuEl.appendChild(paperRippleChild)
  return menuEl
}

function getMenuElement(templateContent, href) {
  let menuEl = templateContent.querySelector(`a[href="${href}"]`)
  if (!menuEl) {
    // Search templates
    const templates = templateContent.querySelectorAll('template')
    for (const template of templates) {
      menuEl = template.content.querySelector(`a[href="${href}"]`)
      if (menuEl) {
        return menuEl
      }
    }
    console.error(`[Brave Settings Overrides] Could not find menu item '${href}'`)
  }
  return menuEl
}

RegisterStyleOverride(
  'settings-menu',
  html`
    <style>
      :host {
        --brave-settings-menu-margin-v: 30px;
        --brave-settings-menu-padding: 30px;
        --settings-nav-item-color: #424242 !important;
        position: sticky;
        top: var(--brave-settings-menu-margin-v);
        margin: 0 var(--brave-settings-menu-margin) !important;
        max-height: calc(100vh - 56px - (var(--brave-settings-menu-margin-v) * 2) - (var(--brave-settings-menu-padding) * 2));
        min-width: 172px;
        border-radius: 6px;
        background-color: #fff;
        overflow-y: auto;
        padding: 30px !important;
      }

      .cr-nav-menu-item {
        min-height: 20px !important;
        border-end-end-radius: 0px !important;
        border-start-end-radius: 0px !important;
        box-sizing: content-box !important;
        overflow: visible !important;
      }

      .cr-nav-menu-item:hover {
        background: transparent !important;
      }

      .cr-nav-menu-item[selected] {
        --selected-gradient-color1: #FA7250;
        --selected-gradient-color2: #FF1893;
        --selected-gradient-color3: #A78AFF;

        color: var(--cr-link-color) !important;
        background: transparent !important;
      }

      .cr-nav-menu-item paper-ripple {
        display: none !important;
      }

      @media (prefers-color-scheme: dark) {
        :host {
          --settings-nav-item-color: #F4F4F4 !important;
          border-color: transparent !important;
          background-color: #161719;
        }
      }

      a[href] {
        font-weight: 400 !important;
        margin: 0 20px 20px 0 !important;
        margin-inline-start: 0 !important;
        margin-inline-end: 0 !important;
        padding-bottom: 0 !important;
        padding-top: 0 !important;
        padding-inline-start: 0 !important;
        position: relative !important;
      }

      a[href]:focus-visible {
        box-shadow: 0 0 0 4px rgba(160, 165, 235, 1) !important;
        outline: none !important;
        border-radius: 6px !important;
      }

      a[href].iron-selected {
        color: #DB2F04;
        font-weight: 400 !important;
      }

      a:hover, iron-icon:hover {
        color: #444DD0 !important;
      }

      iron-icon {
        margin-inline-end: 14px !important;
        width: 24px;
        height: 24px;
      }

      a[href].iron-selected::before {
        content: "";
        position: absolute;
        top: 50%;
        left: calc(-1 * var(--brave-settings-menu-padding));
        transform: translateY(-50%);
        display: block;
        height: 32px;
        width: 4px;
        background: linear-gradient(96.98deg, #E51D00 0%, #E5007B 78.13%);
        border-radius: 0px 2px 2px 0px;
      }

      @media (prefers-color-scheme: dark) {
        a[href].iron-selected {
          color: #FB5930;
        }

        a:hover, iron-icon:hover {
          color: #A6ABE9 !important;
        }
      }

      a[href],
      #advancedButton {
        --cr-selectable-focus_-_outline: var(--brave-focus-outline) !important;
      }

      #advancedButton {
        padding: 0 !important;
        margin-top: 30px !important;
        line-height: 1.25 !important;
        border: none !important;
      }

      #advancedButton > iron-icon {
        margin-inline-end: 0 !important;
      }

      #settingsHeader,
      #advancedButton {
        align-items: center !important;
        font-weight: normal !important;
        font-size: larger !important;
        color: var(--settings-nav-item-color) !important;
        margin-bottom: 20px !important;
      }

      #about-menu {
        display: flex;
        flex-direction: row;
        align-items: flex-start;
        justify-content: flex-start;
        color: #c5c5d3 !important;
        margin: 16px 0 0 0 !important;
      }
      .brave-about-graphic {
        flex: 0;
        flex-basis: 30%;
        display: flex;
        align-items: center;
        justify-content: flex-start;
        align-self: stretch;
      }
      .brave-about-meta {
        flex: 1;
      }
      .brave-about-item {
        display: block;
      }
    </style>
  `
)

RegisterStyleOverride('iron-icon', html`
 <style>
    :host-context(.cr-nav-menu-item) svg {
      fill: url(#selectedGradient);
    }
 </style>`)

RegisterPolymerTemplateModifications({
  'settings-menu': (templateContent) => {
    // Add title
    const titleEl = document.createElement('h1')
    titleEl.id = 'settingsHeader'
    titleEl.textContent = loadTimeData.getString('settings')
    const menuEl = templateContent.querySelector('#menu')
    if (!menuEl) {
      console.error('[Brave Settings Overrides] Could not find menu element to add title after')
    } else {
      menuEl.insertAdjacentElement('afterbegin', titleEl)
    }
    // Add 'Get Started' item
    const peopleEl = getMenuElement(templateContent, '/people')
    const getStartedEl = createMenuElement(
      loadTimeData.getString('braveGetStartedTitle'),
      '/getStarted',
      'brave_settings:get-started',
      'getStarted'
    )
    peopleEl.insertAdjacentElement('afterend', getStartedEl)
    // Move Appearance item
    const appearanceBrowserEl = getMenuElement(templateContent, '/appearance')
    getStartedEl.insertAdjacentElement('afterend', appearanceBrowserEl)

    // Add New Tab item
    const newTabEl = createMenuElement(
      loadTimeData.getString('braveNewTab'),
      '/newTab',
      'brave_settings:new-tab',
      'newTab'
    )
    appearanceBrowserEl.insertAdjacentElement('afterend', newTabEl)
    // Add Shields item
    const shieldsEl = createMenuElement(
      loadTimeData.getString('braveShieldsTitle'),
      '/shields',
      'brave_settings:shields',
      'shields',
    )
    newTabEl.insertAdjacentElement('afterend', shieldsEl)
    // Add Rewards item
    const isBraveRewardsSupported = loadTimeData.getBoolean('isBraveRewardsSupported')
    let rewardsEl = undefined
    if (isBraveRewardsSupported) {
      rewardsEl = createMenuElement(
        loadTimeData.getString('braveRewards'),
        '/rewards',
        'brave_settings:rewards',
        'rewards',
      )
      shieldsEl.insertAdjacentElement('afterend', rewardsEl)
    }
    // Add Embed Blocking item
    const embedEl = createMenuElement(
      loadTimeData.getString('socialBlocking'),
      '/socialBlocking',
      'brave_settings:social-permissions',
      'socialBlocking',
    )
    if (isBraveRewardsSupported) {
      rewardsEl.insertAdjacentElement('afterend', embedEl)
    } else {
      shieldsEl.insertAdjacentElement('afterend', embedEl)
    }
    // Add privacy
    const privacyEl = getMenuElement(templateContent, '/privacy')
    embedEl.insertAdjacentElement('afterend', privacyEl)
    // Add Sync item
    const syncEl = createMenuElement(
      loadTimeData.getString('braveSync'),
      '/braveSync',
      'brave_settings:sync',
      'braveSync',
    )
    privacyEl.insertAdjacentElement('afterend', syncEl)
    // Move search item
    const searchEl = getMenuElement(templateContent, '/search')
    syncEl.insertAdjacentElement('afterend', searchEl)
    // Add Extensions item
    const extensionEl = createMenuElement(
      loadTimeData.getString('braveDefaultExtensions'),
      '/extensions',
      'brave_settings:extensions',
      'extensions',
    )
    searchEl.insertAdjacentElement('afterend', extensionEl)

    const isBraveWalletAllowed = loadTimeData.getBoolean('isBraveWalletAllowed')
    let walletEl = undefined
    if (isBraveWalletAllowed) {
      walletEl = createMenuElement(
        loadTimeData.getString('braveWallet'),
        '/wallet',
        'brave_settings:wallet',
        'wallet',
      )
      extensionEl.insertAdjacentElement('afterend', walletEl)
    }

    const ipfsEl = createMenuElement(
      loadTimeData.getString('braveIPFS'),
      '/ipfs',
      'brave_settings:ipfs',
      'ipfs',
    )
    if (isBraveWalletAllowed) {
      walletEl.insertAdjacentElement('afterend', ipfsEl)
    } else {
      extensionEl.insertAdjacentElement('afterend', ipfsEl)
    }

    // Move autofill to advanced
    const autofillEl = getMenuElement(templateContent, '/autofill')
    const languagesEl = getMenuElement(templateContent, '/languages')
    languagesEl.insertAdjacentElement('beforebegin', autofillEl)
    // Move HelpTips after downloads
    const helpTipsEl = createMenuElement(
      loadTimeData.getString('braveHelpTips'),
      '/braveHelpTips',
      'brave_settings:help',
      'braveHelpTips',
    )
    const downloadsEl = getMenuElement(templateContent, '/downloads')
    downloadsEl.insertAdjacentElement('afterend', helpTipsEl)
    // Allow Accessibility to be removed :-(
    const a11yEl = getMenuElement(templateContent, '/accessibility')
    a11yEl.setAttribute('hidden', '[[!pageVisibility.a11y]')
    // Remove extensions link
    const extensionsLinkEl = templateContent.querySelector('#extensionsLink')
    if (!extensionsLinkEl) {
      console.error('[Brave Settings Overrides] Could not find extensionsLinkEl to remove')
    }
    extensionsLinkEl.remove()
    // Add version number to 'about' link
    const aboutEl = templateContent.querySelector('#about-menu')
    if (!aboutEl) {
      console.error('[Brave Settings Overrides] Could not find about-menu element')
      return
    }
    const parent = aboutEl.parentNode
    parent.removeChild(aboutEl)

    const newAboutEl = document.createElement('a')
    newAboutEl.setAttribute('href', '/help')
    newAboutEl.setAttribute('id', aboutEl.id)

    const graphicsEl = document.createElement('div')
    graphicsEl.setAttribute('class', 'brave-about-graphic')

    const icon = document.createElement('iron-icon')
    icon.setAttribute('icon', 'brave_settings:full-color-brave-lion')

    const metaEl = document.createElement('div')
    metaEl.setAttribute('class', 'brave-about-meta')

    const menuLink = document.createElement('span')
    menuLink.setAttribute('class', 'brave-about-item brave-about-menu-link-text')
    menuLink.textContent = aboutEl.textContent

    const versionEl = document.createElement('span')
    versionEl.setAttribute('class', 'brave-about-item brave-about-menu-version')
    versionEl.textContent = `v ${loadTimeData.getString('braveProductVersion')}`

    parent.appendChild(newAboutEl)
    newAboutEl.appendChild(graphicsEl)
    graphicsEl.appendChild(icon)
    newAboutEl.appendChild(metaEl)
    metaEl.appendChild(menuLink)
    metaEl.appendChild(versionEl)
  }
})
