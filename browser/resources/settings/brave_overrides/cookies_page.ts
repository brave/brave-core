// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-cookies-page': (templateContent) => {
    const privacySandboxSettings4Template = templateContent.
      querySelector(`template[if*='!isPrivacySandboxSettings4_']`)
    if (!privacySandboxSettings4Template) {
      console.error(
        '[Brave Settings Overrides] Could not find template with ' +
        'if*=isPrivacySandboxSettings4_ on cookies page.')
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
    const preloadingToggleTemplate = templateContent.querySelector(
      'template[is=dom-if][if="[[!showPreloadingSubPage_]]"]')
    if (!preloadingToggleTemplate) {
      console.error(
        '[Brave Settings Overrides] Could not find preloading toggle template')
      return
    }
    const networkPredictionToggle = preloadingToggleTemplate.content.
      getElementById('preloadingToggle')
    if (!networkPredictionToggle) {
      console.error(
        '[Brave Settings Overrides] Could not find preloadingToggle id ' +
        'on cookies page.')
    } else {
      networkPredictionToggle.setAttribute('hidden', 'true')
    }
  }
})
