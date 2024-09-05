// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'

import {loadTimeData} from '../i18n_setup.js'

import 'chrome://resources/brave/leo.bundle.js'

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
            icon="autoplay-on">
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
              icon="user">
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
              icon="smartphone-desktop">
            </site-details-permission>
          `)
        const localhostAccessSettings = templateContent.querySelector(
          `div.list-frame > site-details-permission:nth-child(${curChild})`)
        if (!localhostAccessSettings) {
          console.error(
            '[Brave Settings Overrides] Localhost access settings not found')
        } else {
          localhostAccessSettings.setAttribute(
            'label', loadTimeData.getString('siteSettingsLocalhostAccess'))
        }
        curChild++
      }
      // AI Chat feature
      const isOpenLeoFromBraveSearchFeatureEnabled =
        loadTimeData.getBoolean('isOpenLeoFromBraveSearchFeatureEnabled')
      const isOpenAIChatFromBraveSearchEnabled =
        loadTimeData.getBoolean('isOpenAIChatFromBraveSearchEnabled')
      if (isOpenAIChatFromBraveSearchEnabled && isOpenLeoFromBraveSearchFeatureEnabled) {
        firstPermissionItem.insertAdjacentHTML(
          'beforebegin',
          getTrustedHTML`
            <site-details-permission
              category="[[contentSettingsTypesEnum_.BRAVE_OPEN_AI_CHAT]]"
              icon="user">
            </site-details-permission>
          `)
        const braveAIChatSettings = templateContent.
          querySelector(`div.list-frame > site-details-permission:nth-child(${curChild})`)
        if (!braveAIChatSettings) {
          console.error(
            '[Brave Settings Overrides] Couldn\'t find Brave AI chat settings')
        }
        else {
          braveAIChatSettings.setAttribute(
            'label', loadTimeData.getString('siteSettingsBraveOpenAIChat'))
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
              icon="ethereum-on">
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
              icon="solana-on">
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

    // In Chromium, the VR and AR icons are the same but we want to have separate ones.
    templateContent.querySelector('site-details-permission[icon="settings:vr-headset"]')?.setAttribute('icon', 'smartphone-hand')
  }
})
