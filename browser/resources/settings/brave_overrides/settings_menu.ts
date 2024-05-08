// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import '../brave_icons.html.js'

import {RegisterPolymerTemplateModifications, RegisterStyleOverride} from 'chrome://resources/brave/polymer_overriding.js'
import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {loadTimeData} from '../i18n_setup.js'
import 'chrome://resources/brave/leo.bundle.js'

function createMenuElement(title, href, iconName, pageVisibilitySection) {
  const menuEl = document.createElement('a')
  if (pageVisibilitySection) {
    menuEl.setAttribute('hidden', `[[!pageVisibility.${pageVisibilitySection}]]`)
  }
  menuEl.href = href
  menuEl.setAttribute('role', 'menuitem')
  menuEl.setAttribute('class', 'cr-nav-menu-item')

  const icon = document.createElement('iron-icon')
  icon.setAttribute('icon', iconName)
  menuEl.appendChild(icon)

  const text = document.createTextNode(title)
  menuEl.appendChild(text)
  const crRippleChild = document.createElement('cr-ripple')
  menuEl.appendChild(crRippleChild)
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

        --iron-icon-width: 24px;
        --iron-icon-height: 24px;
        --iron-icon-fill-color: currentColor;
      }

      .cr-nav-menu-item:hover {
        background: transparent !important;
      }

      .cr-nav-menu-item[selected] {
        --iron-icon-fill-color: var(--leo-color-icon-interactive);

        color: var(--leo-color-text-interactive) !important;
        background: transparent !important;
      }

      .cr-nav-menu-item cr-ripple {
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

      iron-icon, leo-icon {
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
        background: var(--leo-color-text-interactive);
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

      #autofill {
        margin-top: 20px !important;
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

RegisterPolymerTemplateModifications({
  'settings-menu': (templateContent) => {
    // Hide performance menu. We moved it under system menu instead.
    const performanceEl = getMenuElement(templateContent, '/performance')
    if (performanceEl) {
      performanceEl.remove()
    }

    // Add 'Get Started' item
    const peopleEl = getMenuElement(templateContent, '/people')
    const getStartedEl = createMenuElement(
      loadTimeData.getString('braveGetStartedTitle'),
      '/getStarted',
      'rocket',
      'getStarted'
    )
    peopleEl.insertAdjacentElement('afterend', getStartedEl)

    // Move Appearance item
    const appearanceBrowserEl = getMenuElement(templateContent, '/appearance')
    getStartedEl.insertAdjacentElement('afterend', appearanceBrowserEl)

    // Add Content item
    const contentEl = createMenuElement(
      loadTimeData.getString('contentSettingsContentSection'),
      '/braveContent',
      'content-big',
      'content',
    )
    appearanceBrowserEl.insertAdjacentElement('afterend', contentEl)

    // Add Shields item
    const shieldsEl = createMenuElement(
      loadTimeData.getString('braveShieldsTitle'),
      '/shields',
      'shield-done',
      'shields',
    )
    contentEl.insertAdjacentElement('afterend', shieldsEl)

    // Add privacy item
    const privacyEl = getMenuElement(templateContent, '/privacy')
    shieldsEl.insertAdjacentElement('afterend', privacyEl)

    // Add web3 item
    const web3El = createMenuElement(
      loadTimeData.getString('braveWeb3'),
      '/web3',
      'product-brave-wallet',
      'wallet',
    )
    privacyEl.insertAdjacentElement('afterend', web3El)

    // Add leo item
    const leoAssistantEl = createMenuElement(
      loadTimeData.getString('leoAssistant'),
      '/leo-assistant',
      'product-brave-leo',
      'leoAssistant',
    )
    web3El.insertAdjacentElement('afterend', leoAssistantEl)

    // Add Sync item
    const syncEl = createMenuElement(
      loadTimeData.getString('braveSync'),
      '/braveSync',
      'product-sync',
      'braveSync',
    )
    leoAssistantEl.insertAdjacentElement('afterend', syncEl)

    // Add search item
    const searchEl = getMenuElement(templateContent, '/search')
    syncEl.insertAdjacentElement('afterend', searchEl)

    // Add Extensions item
    const extensionEl = createMenuElement(
      loadTimeData.getString('braveDefaultExtensions'),
      '/extensions',
      'browser-extensions',
      'extensions',
    )
    searchEl.insertAdjacentElement('afterend', extensionEl)

    // Move autofill to advanced
    const autofillEl = getMenuElement(templateContent, '/autofill')
    const languagesEl = getMenuElement(templateContent, '/languages')
    languagesEl.insertAdjacentElement('beforebegin', autofillEl)
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
    newAboutEl.setAttribute('role', 'menuitem')

    const graphicsEl = document.createElement('div')
    graphicsEl.setAttribute('class', 'brave-about-graphic')

    // Use per-channel logo image.
    const icon = document.createElement('img')
    icon.setAttribute('srcset', 'chrome://theme/current-channel-logo@1x, chrome://theme/current-channel-logo@2x 2x')
    icon.setAttribute('width', '24px')
    icon.setAttribute('height', '24px')

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
