// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-cookies-page': (templateContent) => {
    const privacySandboxSettings4Template = templateContent.
      querySelector(
        'template[if*="isPrivacySandboxSettings3CookieSettingsEnabled_"]'
      )
    if (!privacySandboxSettings4Template) {
      console.error(
        '[Brave Settings Overrides] Could not find template with ' +
        'if*=isPrivacySandboxSettings3CookieSettingsEnabled_ on cookies page.')
    } else {
      const clearOnExitToggle =
        privacySandboxSettings4Template.content.getElementById('clearOnExit')
      if (!clearOnExitToggle) {
        console.error(
          '[Brave Settings Overrides] Could not find clearOnExit id ' +
          'on cookies page.')
      } else {
        clearOnExitToggle.setAttribute('sub-label', '')
      }
      const blockThirdPartyIncognitoRadioButton =
        privacySandboxSettings4Template.content.
          getElementById('blockThirdPartyIncognito')
      if (!blockThirdPartyIncognitoRadioButton) {
        console.error(
          '[Brave Settings Overrides] Could not find ' +
          'blockThirdPartyIncognito id on cookies page.')
      } else {
        blockThirdPartyIncognitoRadioButton.setAttribute('hidden', 'true')
      }
    }
    const preloadingLinkRowTemplate = templateContent.querySelector(
        'template[is=dom-if][if="[[showPreloadingSubpage_]]"]')
    if (!preloadingLinkRowTemplate) {
      console.error(
          '[Brave Settings Overrides] Could not find preloading template')
      return
    }
    const networkPredictionLinkRow =
        preloadingLinkRowTemplate.content.getElementById('preloadingLinkRow')
    if (!networkPredictionLinkRow) {
      console.error(
          '[Brave Settings Overrides] Could not find preloadingLinkRow id ' +
          'on cookies page.')
    }
    else {
      networkPredictionLinkRow.setAttribute('hidden', 'true')
    }
  }
})
