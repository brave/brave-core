// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {RegisterPolymerComponentReplacement} from 'chrome://resources/polymer_overriding.js'
import {ContentSettingsTypes} from '../site_settings/constants.js'
import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';
import {SettingsSiteSettingsPageElement} from '../site_settings_page/site_settings_page.js'
import {routes} from '../route.js'
import './config.js'

const PERMISSIONS_BASIC_REMOVE_IDS = [
  ContentSettingsTypes.BACKGROUND_SYNC,
]
const CONTENT_ADVANCED_REMOVE_IDS = [
  ContentSettingsTypes.ADS,
]

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
        const lists_ = oldListsGetter()
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
              icon: 'cr:extension',
              enabledLabel: 'siteSettingsAutoplayAllow',
              disabledLabel: 'siteSettingsAutoplayBlock'
            }
            lists_.permissionsAdvanced.splice(indexForAutoplay, 0, autoplayItem)
            const isNativeBraveWalletEnabled = loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')
            if (isNativeBraveWalletEnabled) {
              let indexForEthereum = indexForAutoplay + 1
              const ethereumItem = {
                route: routes.SITE_SETTINGS_ETHEREUM,
                id: 'ethereum',
                label: 'siteSettingsEthereum',
                icon: 'cr:extension',
                enabledLabel: 'siteSettingsEthereumAsk',
                disabledLabel: 'siteSettingsEthereumBlock'
              }
              lists_.permissionsAdvanced.splice(indexForEthereum, 0, ethereumItem)
              let indexForSolana = indexForEthereum + 1
              const solanaItem = {
                route: routes.SITE_SETTINGS_SOLANA,
                id: 'solana',
                label: 'siteSettingsSolana',
                icon: 'cr:extension',
                enabledLabel: 'siteSettingsSolanaAsk',
                disabledLabel: 'siteSettingsSolanaBlock'
              }
              lists_.permissionsAdvanced.splice(indexForSolana, 0, solanaItem)
            }
          }
        }
        return lists_
      }
      return properties
    }
  }
)
