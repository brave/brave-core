// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  html,
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'

import {loadTimeData} from '../i18n_setup.js'

import '../brave_content_page/content_page_index.js'
import '../getting_started_page/getting_started_page_index.js'
import '../default_brave_shields_page/shields_page_index.js'
import '../brave_wallet_page/wallet_page_index.js'

RegisterPolymerTemplateModifications({
  'settings-main': (templateContent) => {
    // Always show settings-basic-page
    const templatePrivacy = templateContent.querySelector(
      'template[is=dom-if][if="[[showPage_(pageVisibility_.privacy)]]"]')
    if (!templatePrivacy) {
      throw new Error('[Settings] Missing template for privacy')
    }
    const templateSettingsBasicPage =
      templatePrivacy.content.querySelector('#privacy template')
    if (templateSettingsBasicPage) {
      templateSettingsBasicPage.setAttribute('if', 'true')
    } else {
      throw new Error('[Settings] Missing template for settings-basic-page')
    }

    // Remove the performance page template
    const templatePerformancePageSlot = templateContent.querySelector(
      'template[is=dom-if][if="[[showPage_(pageVisibility_.performance)]]"]')
    if (templatePerformancePageSlot) {
      templatePerformancePageSlot.remove()
    } else {
      throw new Error('[Settings] Missing template for performance page slot')
    }

    // Grab the view manager
    const switcher = templateContent.querySelector('#switcher')
    if (!switcher) {
      throw new Error('[Settings] Missing switcher on settings-basic-page')
    }

    // Insert the getStarted page into the view manager
    switcher.appendChild(
      html`
        <template is="dom-if" if="[[showPage_(pageVisibility_.getStarted)]]">
          <div slot="view" id="getStarted">
            <template is="dom-if" if="true">
              <settings-getting-started-page-index
                prefs="{{prefs}}"
                in-search-mode="[[inSearchMode_]]">
              </settings-getting-started-page-index>
            </template>
          </div>
        </template>
      `)

    // Insert the content page into the view manager
    switcher.appendChild(
      html`
        <template is="dom-if" if="[[showPage_(pageVisibility_.content)]]">
          <div slot="view" id="content">
            <template is="dom-if" if="[[renderPlugin_(
          routes_.BRAVE_CONTENT, lastRoute_, inSearchMode_)]]">
              <settings-brave-content-page-index
                prefs="{{prefs}}"
                in-search-mode="[[inSearchMode_]]">
              </settings-brave-content-page-index>
            </template>
          </div>
        </template>
      `)

    // Insert the shields page into the view manager
    switcher.appendChild(
      html`
        <template is="dom-if" if="[[showPage_(pageVisibility_.shields)]]">
          <div slot="view" id="shields">
            <template is="dom-if" if="[[renderPlugin_(
          routes_.SHIELDS, lastRoute_, inSearchMode_)]]">
              <settings-shields-page-index
                prefs="{{prefs}}"
                in-search-mode="[[inSearchMode_]]">
              </settings-shields-page-index>
            </template>
          </div>
        </template>
      `)

    // Insert the web3 page into the view manager
    const isBraveWalletAllowed = loadTimeData.getBoolean('isBraveWalletAllowed')
    if (isBraveWalletAllowed) {
      switcher.appendChild(
        html`
          <template is="dom-if" if="[[showPage_(pageVisibility_.braveWallet)]]">
            <div slot="view" id="web3">
              <template is="dom-if" if="[[renderPlugin_(
          routes_.BRAVE_WEB3, lastRoute_, inSearchMode_)]]">
                <settings-wallet-page-index
                  prefs="{{prefs}}"
                  in-search-mode="[[inSearchMode_]]">
                </settings-wallet-page-index>
              </template>
            </div>
          </template>
        `)
    }

    // Insert the leo page into the view manager
    switcher.appendChild(
      html`
        <template is="dom-if" if="[[showPage_(pageVisibility_.leoAssistant)]]">
          <div slot="view" id="leoAssistant">
            <template is="dom-if" if="true">
              <settings-brave-leo-assistant-page
                prefs="{{prefs}}"
                in-search-mode="[[inSearchMode_]]">
              </settings-brave-leo-assistant-page>
            </template>
            <template is="dom-if" if="[[showPage_(pageVisibility_.leoPersonalization)]]">
              <template is="dom-if" if="true">
                <brave-leo-personalization
                  prefs="{{prefs}}"
                  in-search-mode="[[inSearchMode_]]">
                </brave-leo-personalization>
              </template>
            </template>
            <template is="dom-if" if="[[showPage_(pageVisibility_.leoModels)]]">
              <template is="dom-if" if="true">
                <model-list-section
                  prefs="{{prefs}}"
                  in-search-mode="[[inSearchMode_]]">
                </model-list-section>
              </template>
            </template>
          </div>
        </template>
      `)

    // Insert the sync page into the view manager
    switcher.appendChild(
      html`
        <template is="dom-if" if="[[showPage_(pageVisibility_.braveSync)]]">
          <div slot="view" id="braveSync">
            <template is="dom-if" if="true">
              <settings-brave-sync-page
                prefs="{{prefs}}"
                in-search-mode="[[inSearchMode_]]">
              </settings-brave-sync-page>
            </template>
          </div>
        </template>
      `)

    // Insert the extensions page into the view manager
    switcher.appendChild(
      html`
        <template is="dom-if" if="[[showPage_(pageVisibility_.extensions)]]">
          <div slot="view" id="extensions">
            <template is="dom-if" if="true">
              <settings-brave-default-extensions-page
                prefs="{{prefs}}"
                in-search-mode="[[inSearchMode_]]">
              </settings-brave-default-extensions-page>
            </template>
          </div>
        </template>
      `)
  }
})
