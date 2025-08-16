// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import './config.js'

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js';

import {
  RegisterPolymerComponentReplacement,
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'

import type {CategoryListItem} from '../site_settings_page/site_settings_list.js';

import {loadTimeData} from '../i18n_setup.js'
import {routes} from '../route.js'
import {ContentSettingsTypes} from '../site_settings/constants.js'
import {SettingsSiteSettingsPageElement} from '../site_settings_page/site_settings_page.js'

import '../ad_block_only_mode_page/ad_block_only_mode_alert.js'

const PERMISSIONS_BASIC_REMOVE_IDS = [
  ContentSettingsTypes.STORAGE_ACCESS,
]
const PERMISSIONS_ADVANCED_REMOVE_IDS = [
  ContentSettingsTypes.BACKGROUND_SYNC,
]
const CONTENT_ADVANCED_REMOVE_IDS = [
  ContentSettingsTypes.ADS,
  ContentSettingsTypes.ANTI_ABUSE,
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
        <div class="cr-row first">
          <div class="flex">
            <h2 id="siteSettingsShields"></h2>
            <settings-ad-block-only-mode-alert
              id="adBlockOnlyModeAlert"
              prefs="{{prefs}}">
            </settings-ad-block-only-mode-alert>
          </div>
        </div>
        <settings-site-settings-list id="basicShieldsList"
          category-list="[[braveLists_.shieldsBasic]]"
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
  class BraveSiteSettingsComponent extends PrefsMixin(SettingsSiteSettingsPageElement) {
    declare private braveLists_: {
      shieldsBasic: CategoryListItem[],
    }

    static override get properties() {
      const properties = SettingsSiteSettingsPageElement.properties
      if (!properties || !properties.lists_ || !properties.lists_.value) {
        console.error('[Settings] Could not find polymer lists_ property')
      }
      const oldListsGetter = properties.lists_.value
      properties.lists_.value = function () {
        const lists = oldListsGetter()
        if (!lists.permissionsBasic) {
          console.error('[Settings] did not get lists.permissionsBasic data')
        } else {
          lists.permissionsBasic = lists.permissionsBasic.filter(
            item => !PERMISSIONS_BASIC_REMOVE_IDS.includes(item.id))
        }
        if (!lists.contentAdvanced) {
          console.error('[Settings] did not get lists.contentAdvanced data')
        } else {
          lists.contentAdvanced = lists.contentAdvanced.filter(
            item => !CONTENT_ADVANCED_REMOVE_IDS.includes(item.id))
        }
        if (!lists.permissionsAdvanced) {
          console.error(
            '[Settings] did not get lists.permissionsAdvanced data')
        } else {
          lists.permissionsAdvanced = lists.permissionsAdvanced.filter(
            item => !PERMISSIONS_ADVANCED_REMOVE_IDS.includes(item.id))
          if (!loadTimeData.getBoolean('isIdleDetectionFeatureEnabled')) {
            const indexForIdleDetection = lists.permissionsAdvanced.findIndex(
              item => item.id === ContentSettingsTypes.IDLE_DETECTION)
            if (indexForIdleDetection === -1) {
              console.error('Could not find idle detection site settings item')
            } else {
              lists.permissionsAdvanced.splice(indexForIdleDetection, 1)
            }
          }
          let indexForAutoplay = lists.permissionsAdvanced.findIndex(
            item => item.id === ContentSettingsTypes.AUTOMATIC_DOWNLOADS)
          if (indexForAutoplay === -1) {
            console.error('Could not find automatic downloads site settings item')
          } else {
            indexForAutoplay++
            const autoplayItem = {
              route: routes.SITE_SETTINGS_AUTOPLAY,
              id: ContentSettingsTypes.AUTOPLAY,
              label: 'siteSettingsAutoplay',
              icon: 'autoplay-on',
              enabledLabel: 'siteSettingsAutoplayAllow',
              disabledLabel: 'siteSettingsAutoplayBlock'
            }
            lists.permissionsAdvanced.splice(indexForAutoplay, 0, autoplayItem)
            let currentIndex = indexForAutoplay
            const isGoogleSignInFeatureEnabled =
              loadTimeData.getBoolean('isGoogleSignInFeatureEnabled')
            if (isGoogleSignInFeatureEnabled) {
              currentIndex++
              const googleSignInItem = {
                route: routes.SITE_SETTINGS_GOOGLE_SIGN_IN,
                id: ContentSettingsTypes.GOOGLE_SIGN_IN,
                label: 'siteSettingsGoogleSignIn',
                icon: 'user',
                enabledLabel: 'siteSettingsGoogleSignInAsk',
                disabledLabel: 'siteSettingsGoogleSignInBlock'
              }
              lists.permissionsAdvanced.splice(currentIndex, 0,
                googleSignInItem)
            }
            const isLocalhostAccessFeatureEnabled =
              loadTimeData.getBoolean('isLocalhostAccessFeatureEnabled')
            if (isLocalhostAccessFeatureEnabled) {
              currentIndex++
              const localhostAccessItem = {
                route: routes.SITE_SETTINGS_LOCALHOST_ACCESS,
                id: ContentSettingsTypes.LOCALHOST_ACCESS,
                label: 'siteSettingsLocalhostAccess',
                icon: 'smartphone-desktop',
                enabledLabel: 'siteSettingsLocalhostAccessAsk',
                disabledLabel: 'siteSettingsLocalhostAccessBlock'
              }
              lists.permissionsAdvanced.splice(currentIndex, 0,
                localhostAccessItem)
            }
            const isOpenAIChatFromBraveSearchEnabled =
              loadTimeData.getBoolean('isOpenAIChatFromBraveSearchEnabled')
            if (isOpenAIChatFromBraveSearchEnabled) {
              currentIndex++
              const AIChatItem = {
                route: routes.SITE_SETTINGS_BRAVE_OPEN_AI_CHAT,
                id: ContentSettingsTypes.BRAVE_OPEN_AI_CHAT,
                label: 'siteSettingsBraveOpenAIChat',
                icon: 'product-brave-leo',
                enabledLabel: 'siteSettingsBraveOpenAIChatAsk',
                disabledLabel: 'siteSettingsBraveOpenAIChatBlock'
              }
              lists.permissionsAdvanced.splice(currentIndex, 0,
                AIChatItem)
            }
            const isNativeBraveWalletEnabled =
              loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')
            const isCardanoDappSupportFeatureEnabled =
              loadTimeData.getBoolean('isCardanoDappSupportFeatureEnabled')
            const isBraveWalletAllowed =
              loadTimeData.getBoolean('isBraveWalletAllowed')
            if (isNativeBraveWalletEnabled && isBraveWalletAllowed) {
              currentIndex++
              const ethereumItem = {
                route: routes.SITE_SETTINGS_ETHEREUM,
                id: ContentSettingsTypes.ETHEREUM,
                label: 'siteSettingsEthereum',
                icon: 'ethereum-on',
                enabledLabel: 'siteSettingsEthereumAsk',
                disabledLabel: 'siteSettingsEthereumBlock'
              }
              lists.permissionsAdvanced.splice(currentIndex, 0, ethereumItem)
              currentIndex++
              const solanaItem = {
                route: routes.SITE_SETTINGS_SOLANA,
                id: ContentSettingsTypes.SOLANA,
                label: 'siteSettingsSolana',
                icon: 'solana-on',
                enabledLabel: 'siteSettingsSolanaAsk',
                disabledLabel: 'siteSettingsSolanaBlock'
              }
              lists.permissionsAdvanced.splice(currentIndex, 0, solanaItem)
              if (isCardanoDappSupportFeatureEnabled) {
                currentIndex++
                const cardanoItem = {
                  route: routes.SITE_SETTINGS_CARDANO,
                  id: ContentSettingsTypes.CARDANO,
                  label: 'siteSettingsCardano',
                  icon: 'cardano-on',
                  enabledLabel: 'siteSettingsCardanoAsk',
                  disabledLabel: 'siteSettingsCardanoBlock'
                }
                lists.permissionsAdvanced.splice(currentIndex, 0, cardanoItem)
              }
            }
          }
        }

        return lists
      }

      return {
        ...properties,
        braveLists_: {
          type: Object,
          value: function () {
            return {
              shieldsBasic: [
                {
                  route: routes.SITE_SETTINGS_SHIELDS_STATUS,
                  id: ContentSettingsTypes.BRAVE_SHIELDS,
                  label: 'siteSettingsShieldsStatus',
                  icon: 'shield-done',
                  enabledLabel: 'siteSettingsShieldsDescription',
                  disabledLabel: 'siteSettingsShieldsDown'
                }
              ]
            }
          }
        }
      }
    }

    static get observers() {
      return [
        'updateShieldsLabel_(prefs.brave.ad_block.adblock_only_mode_enabled.value)'
      ]
    }

    updateShieldsLabel_() {
      const index =
        this.braveLists_.shieldsBasic.map(e => e.id).indexOf(ContentSettingsTypes.BRAVE_SHIELDS);
      // The third-party cookies might not be part of the current
      // site-settings-list but the class always observes the preference.
      if (index === -1) {
        return;
      }

      if (this.getPref('brave.ad_block.adblock_only_mode_enabled').value) {
        this.braveLists_.shieldsBasic[index].enabledLabel =
            'siteSettingsShieldsDescriptionAdblockOnlyMode';
      } else {
        this.braveLists_.shieldsBasic[index].enabledLabel =
            'siteSettingsShieldsDescription';
      }
    }
  }
)
