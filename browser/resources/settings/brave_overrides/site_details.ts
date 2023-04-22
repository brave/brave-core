// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {RegisterPolymerTemplateModifications,RegisterPolymerComponentReplacement} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'

import {loadTimeData} from '../i18n_setup.js'
import {BraveSiteDetailsElement} from '../brave_site_details/brave_site_details.js'

RegisterPolymerComponentReplacement(
  'site-details',
  BraveSiteDetailsElement
)

RegisterPolymerTemplateModifications({
  'site-details': (templateContent) => {
    if (!loadTimeData.getBoolean('isIdleDetectionFeatureEnabled')) {
      const idleDetectionItem = templateContent.querySelector('[category="[[contentSettingsTypesEnum_.IDLE_DETECTION]]"]')
      if (!idleDetectionItem) {
        console.error(`[Brave Settings Overrides] Couldn't find idle detection item`)
      } else {
        idleDetectionItem.hidden = true
      }
    }
    const adsItem = templateContent.querySelector('[category="[[contentSettingsTypesEnum_.ADS]]"]')
    if (!adsItem) {
      console.error(`[Brave Settings Overrides] Couldn't find ads item`)
    } else {
      adsItem.hidden = true
    }
    const firstPermissionItem = templateContent.querySelector('div.list-frame > site-details-permission:nth-child(1)')
    if (!firstPermissionItem) {
      console.error(`[Brave Settings Overrides] Couldn't find first permission item`)
    } else {
      firstPermissionItem.insertAdjacentHTML(
        'beforebegin',
        getTrustedHTML`
          <site-details-permission
            category="[[contentSettingsTypesEnum_.AUTOPLAY]]"
            icon="cr:extension">
          </site-details-permission>
        `)
      let curChild = 1
      const autoplaySettings = templateContent.
        querySelector(`div.list-frame > site-details-permission:nth-child(${curChild})`)
      if (!autoplaySettings) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find autoplay settings')
      }
      else {
        autoplaySettings.setAttribute(
          'label', loadTimeData.getString('siteSettingsAutoplay'))
      }
      curChild++
      // Google Sign-In feature
      const isGoogleSignInFeatureEnabled =
        loadTimeData.getBoolean('isGoogleSignInFeatureEnabled')
      if (isGoogleSignInFeatureEnabled) {
        firstPermissionItem.insertAdjacentHTML(
          'beforebegin',
          getTrustedHTML`
            <site-details-permission
              category="[[contentSettingsTypesEnum_.GOOGLE_SIGN_IN]]"
              icon="cr:person">
            </site-details-permission>
          `)
        const googleSignInSettings = templateContent.
          querySelector(`div.list-frame > site-details-permission:nth-child(${curChild})`)
        if (!googleSignInSettings) {
          console.error(
            '[Brave Settings Overrides] Couldn\'t find Google signin settings')
        }
        else {
          googleSignInSettings.setAttribute(
            'label', loadTimeData.getString('siteSettingsGoogleSignIn'))
        }
        curChild++
      }
      // Localhost Access feature
      const isLocalhostAccessFeatureEnabled =
        loadTimeData.getBoolean('isLocalhostAccessFeatureEnabled')
      if (isLocalhostAccessFeatureEnabled) {
        firstPermissionItem.insertAdjacentHTML(
          'beforebegin',
          getTrustedHTML`
            <site-details-permission
              category="[[contentSettingsTypesEnum_.LOCALHOST_ACCESS]]"
              icon="settings:devices">
            </site-details-permission>
          `)
        const localhostAccessSettings = templateContent.querySelector(
          `div.list-frame > site-details-permission:nth-child(${curChild})`)
        if (!localhostAccessSettings) {
          console.error(
            '[Brave Settings Overrides] Localhost access settings not found')
        } else {
          localhostAccessSettings.setAttribute(
            'label', I18nBehavior.i18n('siteSettingsLocalhostAccess'))
        }
        curChild++
      }
      const isNativeBraveWalletEnabled = loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')
      if (isNativeBraveWalletEnabled) {
        firstPermissionItem.insertAdjacentHTML(
          'beforebegin',
          getTrustedHTML`
            <site-details-permission
              category="[[contentSettingsTypesEnum_.ETHEREUM]]"
              icon="cr:extension">
            </site-details-permission>
          `)
        const ethereumSettings = templateContent.
          querySelector(`div.list-frame > site-details-permission:nth-child(${curChild})`)
        if (!ethereumSettings) {
          console.error(
              '[Brave Settings Overrides] Couldn\'t find Ethereum settings')
        } else {
          ethereumSettings.setAttribute(
            'label', loadTimeData.getString('siteSettingsEthereum'))
        }
        curChild++
        firstPermissionItem.insertAdjacentHTML(
          'beforebegin',
          getTrustedHTML`
            <site-details-permission
              category="[[contentSettingsTypesEnum_.SOLANA]]"
              icon="cr:extension">
            </site-details-permission>
          `)
        const solanaSettings = templateContent.
          querySelector(`div.list-frame > site-details-permission:nth-child(${curChild})`)
        if (!solanaSettings) {
          console.error(
            '[Brave Settings Overrides] Couldn\'t find Solana settings')
        } else {
          solanaSettings.setAttribute(
              'label', loadTimeData.getString('siteSettingsSolana'))
        }
      }
    }
    const usageSection = templateContent.querySelector('div#usage')
    if (!usageSection) {
      console.error(`[Brave Settings Overrides] Couldn't find usageSection item`)
    } else {
      usageSection.insertAdjacentHTML(
        'afterend',
        getTrustedHTML`
          <div id="shields">
            <div id="shieldsHeader"
              style="padding: 0 var(--cr-section-padding);">
              <h2 class="first"></h2>
            </div>
            <div class="list-frame">
              <site-details-permission
                category="[[contentSettingsTypesEnum_.BRAVE_SHIELDS]]">
              </site-details-permission>
            </div>
          </div>
        `)
      const shieldsHeader =
        templateContent.querySelector('div#shieldsHeader h2')
      if (!shieldsHeader) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Shields header')
      } else {
        shieldsHeader.textContent =
            loadTimeData.getString('siteSettingsShields')
      }
      const shieldsSettings = templateContent.querySelector(
        'div#shields div.list-frame > site-details-permission:nth-child(1)')
      if (!shieldsSettings) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Shields settings')
      } else {
        shieldsSettings.setAttribute(
            'label', loadTimeData.getString('siteSettingsShieldsStatus'))
      }
      usageSection.insertAdjacentHTML(
        'afterend',
        getTrustedHTML`

          <cr-link-row id="cookiesLink" on-click="onCookiesDetailClicked_">
          </cr-link-row>

        `)
      const cookiesDetail =
        templateContent.querySelector('#cookiesLink')
      if (!cookiesDetail) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find cookies link')
      } else {
        cookiesDetail.setAttribute('label',
            loadTimeData.getString('siteCookiesLinkLabel'))
        cookiesDetail.setAttribute('sub-label',
            loadTimeData.getString('siteCookiesLinkDesc'))
      }
    }
  }
})
