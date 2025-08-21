// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {pageVisibility} from './brave_overrides/page_visibility.js'
import {loadTimeData} from './i18n_setup.js'
import {SettingsRoutes} from './router.js'

export default function addBraveRoutes(r: Partial<SettingsRoutes>) {
  const isGuest = loadTimeData.getBoolean('isGuest')
  if (!r.BASIC) {
    console.error('[Settings] Routes: could not find BASIC page')
    return
  }
  if (pageVisibility.getStarted) {
    r.GET_STARTED = r.BASIC.createSection('/getStarted', 'getStarted')
    r.GET_STARTED.hasMigratedToPlugin = true

    // Bring back people's /manageProfile (now in getStarted)
    if (!r.MANAGE_PROFILE) {
      r.MANAGE_PROFILE = r.GET_STARTED.createChild('/manageProfile')
      r.MANAGE_PROFILE.hasMigratedToPlugin = true
      r.MANAGE_PROFILE.section = 'getStarted'
    }
    // We re-section people page into getStarted section (see people_page Brave
    // override), so we need to adjust the route accordingly in order for the
    // direct navigation to brave://settings/importData to work.
    if (!r.IMPORT_DATA) {
      r.IMPORT_DATA = r.GET_STARTED.createChild('/importData')
      r.IMPORT_DATA.isNavigableDialog = true
      r.IMPORT_DATA.hasMigratedToPlugin = true
      r.IMPORT_DATA.section = 'getStarted'
    }
  }
  r.SHIELDS = r.BASIC.createSection('/shields', 'shields')
  r.SHIELDS.hasMigratedToPlugin = true
  r.SHIELDS_ADBLOCK = r.SHIELDS.createChild('/shields/filters')
  r.SHIELDS_ADBLOCK.hasMigratedToPlugin = true

  if (loadTimeData.getBoolean('areShortcutsSupported')) {
    if (r.SYSTEM) {
      r.SHORTCUTS = r.SYSTEM.createChild('/system/shortcuts')
      r.SHORTCUTS.hasMigratedToPlugin = true
    } else if (!isGuest) {
      console.error('[Settings] Routes: could not find SYSTEM page')
    }
  }
  r.SOCIAL_BLOCKING = r.BASIC.createSection('/socialBlocking', 'socialBlocking')
  r.SOCIAL_BLOCKING.hasMigratedToPlugin = true

  r.EXTENSIONS = r.BASIC.createSection('/extensions', 'extensions')
  r.EXTENSIONS.hasMigratedToPlugin = true
  r.EXTENSIONS_V2 = r.EXTENSIONS.createChild('/extensions/v2')
  r.EXTENSIONS_V2.hasMigratedToPlugin = true
  if (pageVisibility.braveSync) {
    r.BRAVE_SYNC = r.BASIC.createSection('/braveSync', 'braveSync')
    r.BRAVE_SYNC.hasMigratedToPlugin = true
    r.BRAVE_SYNC_SETUP = r.BRAVE_SYNC.createChild('/braveSync/setup')
    r.BRAVE_SYNC_SETUP.hasMigratedToPlugin = true
  }
  if (pageVisibility.braveWallet) {
    r.BRAVE_WEB3 = r.BASIC.createSection('/web3', 'web3')
    r.BRAVE_WEB3.hasMigratedToPlugin = true

    r.BRAVE_WALLET_NETWORKS = r.BRAVE_WEB3.createChild('/wallet/networks')
    r.BRAVE_WALLET_NETWORKS.hasMigratedToPlugin = true
  }
  r.BRAVE_NEW_TAB = r.BASIC.createSection('/newTab', 'newTab')

  if (pageVisibility.leoAssistant) {
    r.BRAVE_LEO_ASSISTANT =
      r.BASIC.createSection('/leo-ai', 'leoAssistant')
    r.BRAVE_LEO_ASSISTANT.hasMigratedToPlugin = true

    r.BRAVE_LEO_CUSTOMIZATION = r.BRAVE_LEO_ASSISTANT
      .createChild('/leo-ai/customization')
    r.BRAVE_LEO_CUSTOMIZATION.hasMigratedToPlugin = true
  }
  if (pageVisibility.content) {
    r.BRAVE_CONTENT = r.BASIC.createSection('/braveContent', 'content')
    r.BRAVE_CONTENT.hasMigratedToPlugin = true

    // Move fonts from APPEARANCE to BRAVE_CONTENT
    if (r.FONTS) {
        delete r.FONTS
    }
    r.FONTS = r.BRAVE_CONTENT.createChild('/fonts')
    r.FONTS.hasMigratedToPlugin = true
  }
  if (pageVisibility.surveyPanelist) {
    r.BRAVE_SURVEY_PANELIST =
      r.BASIC.createSection('/surveyPanelist', 'surveyPanelist')
  }
  if (r.SEARCH) {
    r.DEFAULT_SEARCH = r.SEARCH.createChild('defaultSearch')
    r.DEFAULT_SEARCH.isNavigableDialog = true
    r.PRIVATE_SEARCH = r.SEARCH.createChild('privateSearch')
    r.PRIVATE_SEARCH.isNavigableDialog = true
  }
  if (r.SITE_SETTINGS) {
    r.SITE_SETTINGS_AUTOPLAY = r.SITE_SETTINGS.createChild('autoplay')
    const isGoogleSignInFeatureEnabled =
      loadTimeData.getBoolean('isGoogleSignInFeatureEnabled')
    if (isGoogleSignInFeatureEnabled) {
      r.SITE_SETTINGS_GOOGLE_SIGN_IN =
        r.SITE_SETTINGS.createChild('googleSignIn')
    }
    const isLocalhostAccessFeatureEnabled =
      loadTimeData.getBoolean('isLocalhostAccessFeatureEnabled')
    if (isLocalhostAccessFeatureEnabled) {
      r.SITE_SETTINGS_LOCALHOST_ACCESS = r.SITE_SETTINGS
        .createChild('localhostAccess')
    }
    const isOpenAIChatFromBraveSearchEnabled =
      loadTimeData.getBoolean('isOpenAIChatFromBraveSearchEnabled')
    if (isOpenAIChatFromBraveSearchEnabled) {
      r.SITE_SETTINGS_BRAVE_OPEN_AI_CHAT =
        r.SITE_SETTINGS.createChild('braveOpenAIChat')
    }
    const isNativeBraveWalletFeatureEnabled =
      loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')
    const isCardanoDappSupportFeatureEnabled =
      loadTimeData.getBoolean('isCardanoDappSupportFeatureEnabled')
    const isBraveWalletAllowed = loadTimeData.getBoolean('isBraveWalletAllowed')
    if (isNativeBraveWalletFeatureEnabled && isBraveWalletAllowed) {
      r.SITE_SETTINGS_ETHEREUM = r.SITE_SETTINGS.createChild('ethereum')
      r.SITE_SETTINGS_SOLANA = r.SITE_SETTINGS.createChild('solana')
      if (isCardanoDappSupportFeatureEnabled) {
        r.SITE_SETTINGS_CARDANO = r.SITE_SETTINGS.createChild('cardano')
      }
    }
    r.SITE_SETTINGS_SHIELDS_STATUS = r.SITE_SETTINGS.createChild('braveShields')
    if (r.SITE_SETTINGS_ADS) {
      delete r.SITE_SETTINGS_ADS
    } else {
      console.error(
        '[Settings] could not find expected route site_settings_ads')
    }
    if (r.SITE_SETTINGS_AUTO_VERIFY) {
      delete r.SITE_SETTINGS_AUTO_VERIFY
    } else {
      console.error(
        '[Settings] could not find expected route site_settings_auto_verify')
    }
  } else if (!isGuest) {
    console.error('[Settings] Routes: could not find SITE_SETTINGS page')
  }
  // Autofill route is moved to advanced,
  // otherwise its sections won't show up when opened.
  if (r.AUTOFILL && r.ADVANCED) {
    r.AUTOFILL.parent = r.ADVANCED
  } else if (!isGuest) {
    console.error(
      '[Settings] Could not move autofill route to advanced route', r)
  }
  if (loadTimeData.getBoolean('isEmailAliasesEnabled') && r.AUTOFILL) {
    r.EMAIL_ALIASES = r.AUTOFILL.createChild('/email-aliases')
  }
  // Delete performance menu - system menu includes it instead.
  if (r.PERFORMANCE) {
    delete r.PERFORMANCE
  }
  // Delete storage access
  if (r.SITE_SETTINGS_STORAGE_ACCESS) {
    delete r.SITE_SETTINGS_STORAGE_ACCESS
  }
  if (r.SYNC) {
    delete r.SYNC
  }
  // Delete /syncSetup/advanced
  if (r.SYNC_ADVANCED) {
    delete r.SYNC_ADVANCED
  }
}
