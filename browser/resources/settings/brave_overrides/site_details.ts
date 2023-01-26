// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {RegisterPolymerTemplateModifications} from 'chrome://resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/i18n_behavior.js'
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'

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
      const autoplaySettings = templateContent.
        querySelector('div.list-frame > site-details-permission:nth-child(1)')
      if (!autoplaySettings) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find autoplay settings')
      } else {
        autoplaySettings.setAttribute(
          'label', I18nBehavior.i18n('siteSettingsAutoplay'))
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
          querySelector('div.list-frame > site-details-permission:nth-child(3)')
        if (!ethereumSettings) {
          console.error(
            '[Brave Settings Overrides] Couldn\'t find Ethereum settings')
        } else {
          ethereumSettings.setAttribute(
            'label', I18nBehavior.i18n('siteSettingsEthereum'))
        }
        firstPermissionItem.insertAdjacentHTML(
          'beforebegin',
          getTrustedHTML`
            <site-details-permission
              category="[[contentSettingsTypesEnum_.SOLANA]]"
              icon="cr:extension">
            </site-details-permission>
          `)
        const solanaSettings = templateContent.
          querySelector('div.list-frame > site-details-permission:nth-child(4)')
        if (!solanaSettings) {
          console.error(
            '[Brave Settings Overrides] Couldn\'t find Solana settings')
        } else {
          solanaSettings.setAttribute(
            'label', I18nBehavior.i18n('siteSettingsSolana'))
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
        shieldsHeader.textContent = I18nBehavior.i18n('siteSettingsShields')
      }
      const shieldsSettings = templateContent.querySelector(
        'div#shields div.list-frame > site-details-permission:nth-child(1)')
      if (!shieldsSettings) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find Shields settings')
      } else {
        shieldsSettings.setAttribute(
          'label', I18nBehavior.i18n('siteSettingsShieldsStatus'))
      }
    }
  }
})
