// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import './config.js'

import {RegisterPolymerComponentReplacement, RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'

import {loadTimeData} from '../i18n_setup.js'
import {routes} from '../route.js'
import {ContentSettingsTypes} from '../site_settings/constants.js'
import {SettingsSiteSettingsPageElement} from '../site_settings_page/site_settings_page.js'

const PERMISSIONS_BASIC_REMOVE_IDS = [
  ContentSettingsTypes.STORAGE_ACCESS,
]
const PERMISSIONS_ADVANCED_REMOVE_IDS = [
  ContentSettingsTypes.BACKGROUND_SYNC,
]
const CONTENT_ADVANCED_REMOVE_IDS = [
  ContentSettingsTypes.ADS,
  ContentSettingsTypes.PERFORMANCE,
]

RegisterPolymerTemplateModifications({
  'settings-site-settings-page': (templateContent) => {
    const allSites = templateContent.querySelector('#allSites')
    if (!allSites) {
      console.error('[Settings] Could not find all sites list')
      return
    }
    allSites.insertAdjacentHTML(
      'afterend',
      getTrustedHTML`
        <div class="cr-row first line-only">
          <h2 id="siteSettingsShields"></h2>
        </div>
        <settings-site-settings-list id="basicShieldsList"
          category-list="[[lists_.shieldsBasic]]"
          focus-config="[[focusConfig]]">
        </settings-site-settings-list>
      `)
    const siteSettingsShieldsTitle =
      templateContent.getElementById('siteSettingsShields')
    if (!siteSettingsShieldsTitle) {
      console.error('[Settings] Couldn\'t find shields title')
    } else {
      siteSettingsShieldsTitle.textContent =
          loadTimeData.getString('siteSettingsShields')
    }
  }
})

RegisterPolymerComponentReplacement(
  'settings-site-settings-page',
  class BraveComponent extends SettingsSiteSettingsPageElement {
    static get properties() {
      const properties = SettingsSiteSettingsPageElement.properties
      if (!properties || !properties.lists_ || !properties.lists_.value) {
        console.error('[Brave Settings Overrides] Could not find polymer lists_ property')
        return
      }
      const oldListsGetter = properties.lists_.value
      properties.lists_.value = function () {
        let lists_ = oldListsGetter()
        if (!lists_) {
          console.error('[Brave Settings Overrides] did not get lists_ data')
          return
        }
        if (!lists_.permissionsBasic) {
          console.error('[Brave Settings Overrides] did not get lists_.permissionsBasic data')
        } else {
          lists_.permissionsBasic = lists_.permissionsBasic.filter(item => !PERMISSIONS_BASIC_REMOVE_IDS.includes(item.id))
        }
        if (!lists_.contentAdvanced) {
          console.error('[Brave Settings Overrides] did not get lists_.contentAdvanced data')
        } else {
          lists_.contentAdvanced = lists_.contentAdvanced.filter(item => !CONTENT_ADVANCED_REMOVE_IDS.includes(item.id))
        }
        if (!lists_.permissionsAdvanced) {
          console.error('[Brave Settings Overrides] did not get lists_.permissionsAdvanced data')
        } else {
          lists_.permissionsAdvanced = lists_.permissionsAdvanced.filter(item => !PERMISSIONS_ADVANCED_REMOVE_IDS.includes(item.id))
          if (!loadTimeData.getBoolean('isIdleDetectionFeatureEnabled')) {
            let indexForIdleDetection = lists_.permissionsAdvanced.findIndex(item => item.id === ContentSettingsTypes.IDLE_DETECTION)
            if (indexForIdleDetection === -1) {
              console.error('Could not find idle detection site settings item')
            } else {
              lists_.permissionsAdvanced.splice(indexForIdleDetection, 1)
            }
          }
          let indexForAutoplay = lists_.permissionsAdvanced.findIndex(item => item.id === ContentSettingsTypes.AUTOMATIC_DOWNLOADS)
          if (indexForAutoplay === -1) {
            console.error('Could not find automatic downloads site settings item')
          } else {
            indexForAutoplay++
            const autoplayItem = {
              route: routes.SITE_SETTINGS_AUTOPLAY,
              id: 'autoplay',
              label: 'siteSettingsAutoplay',
              icon: 'autoplay-on',
              enabledLabel: 'siteSettingsAutoplayAllow',
              disabledLabel: 'siteSettingsAutoplayBlock'
            }
            lists_.permissionsAdvanced.splice(indexForAutoplay, 0, autoplayItem)
            let currentIndex = indexForAutoplay
            const isGoogleSignInFeatureEnabled =
              loadTimeData.getBoolean('isGoogleSignInFeatureEnabled')
            if (isGoogleSignInFeatureEnabled) {
              currentIndex++
              const googleSignInItem = {
                route: routes.SITE_SETTINGS_GOOGLE_SIGN_IN,
                id: 'googleSignIn',
                label: 'siteSettingsGoogleSignIn',
                icon: 'user',
                enabledLabel: 'siteSettingsGoogleSignInAsk',
                disabledLabel: 'siteSettingsGoogleSignInBlock'
              }
              lists_.permissionsAdvanced.splice(currentIndex, 0,
                googleSignInItem)
            }
            const isLocalhostAccessFeatureEnabled =
              loadTimeData.getBoolean('isLocalhostAccessFeatureEnabled')
            if (isLocalhostAccessFeatureEnabled) {
              currentIndex++
              const localhostAccessItem = {
                route: routes.SITE_SETTINGS_LOCALHOST_ACCESS,
                id: 'localhostAccess',
                label: 'siteSettingsLocalhostAccess',
                icon: 'smartphone-desktop',
                enabledLabel: 'siteSettingsLocalhostAccessAsk',
                disabledLabel: 'siteSettingsLocalhostAccessBlock'
              }
              lists_.permissionsAdvanced.splice(currentIndex, 0,
                localhostAccessItem)
            }
            const isOpenAIChatFromBraveSearchEnabled =
              loadTimeData.getBoolean('isOpenAIChatFromBraveSearchEnabled')
            if (isOpenAIChatFromBraveSearchEnabled) {
              currentIndex++
              const AIChatItem = {
                route: routes.SITE_SETTINGS_BRAVE_OPEN_AI_CHAT,
                id: 'braveOpenAIChat',
                label: 'siteSettingsBraveOpenAIChat',
                icon: 'product-brave-leo',
                enabledLabel: 'siteSettingsBraveOpenAIChatAsk',
                disabledLabel: 'siteSettingsBraveOpenAIChatBlock'
              }
              lists_.permissionsAdvanced.splice(currentIndex, 0,
                AIChatItem)
            }
            const isNativeBraveWalletEnabled =
              loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')
            if (isNativeBraveWalletEnabled) {
              currentIndex++
              const ethereumItem = {
                route: routes.SITE_SETTINGS_ETHEREUM,
                id: 'ethereum',
                label: 'siteSettingsEthereum',
                icon: 'ethereum-on',
                enabledLabel: 'siteSettingsEthereumAsk',
                disabledLabel: 'siteSettingsEthereumBlock'
              }
              lists_.permissionsAdvanced.splice(currentIndex, 0, ethereumItem)
              currentIndex++
              const solanaItem = {
                route: routes.SITE_SETTINGS_SOLANA,
                id: 'solana',
                label: 'siteSettingsSolana',
                icon: 'solana-on',
                enabledLabel: 'siteSettingsSolanaAsk',
                disabledLabel: 'siteSettingsSolanaBlock'
              }
              lists_.permissionsAdvanced.splice(currentIndex, 0, solanaItem)
            }
          }
        }
        lists_.shieldsBasic = [
          {
            route: routes.SITE_SETTINGS_SHIELDS_STATUS,
            id: 'braveShields',
            label: 'siteSettingsShieldsStatus',
            icon: 'shield-done',
            enabledLabel: 'siteSettingsShieldsDescription',
            disabledLabel: 'siteSettingsShieldsDown'
          }
        ]

        return lists_
      }
      return properties
    }
  }
)
