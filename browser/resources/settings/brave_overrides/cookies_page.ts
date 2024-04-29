// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-cookies-page': (templateContent) => {
    const isNot3pcdRedesignEnabledTemplate = templateContent.
      querySelector(
        'template[if*="!is3pcdRedesignEnabled_"]'
      )
    if (!isNot3pcdRedesignEnabledTemplate) {
      console.error(
        '[Brave Settings Overrides] Could not find template with ' +
        'if*=!is3pcdRedesignEnabledTemplate on cookies page.')
    } else {
      const blockThirdPartyIncognitoRadioButton =
        isNot3pcdRedesignEnabledTemplate.content.
          getElementById('blockThirdPartyIncognito')
      if (!blockThirdPartyIncognitoRadioButton) {
        console.error(
          '[Brave Settings Overrides] Could not find ' +
          'blockThirdPartyIncognito id on cookies page.')
      } else {
        blockThirdPartyIncognitoRadioButton.setAttribute('hidden', 'true')
      }
      const generalControls = isNot3pcdRedesignEnabledTemplate.content.
          getElementById('generalControls')
      if (!generalControls) {
        console.error(
          '[Brave Settings Overrides] Could not find generalControls id ' +
          'on cookies page.')
      } else {
        generalControls.setAttribute('hidden', 'true')
      }
    }
    const siteDataTrigger = templateContent.getElementById('site-data-trigger')
    if (!siteDataTrigger) {
      console.error(
        '[Brave Settings Overrides] Could not find site-data-trigger id ' +
        'on cookies page')
    } else {
      siteDataTrigger.setAttribute('hidden', 'true')
    }
    const doNotTrackToggle = templateContent.getElementById('doNotTrack')
    if (!doNotTrackToggle) {
      console.error(
        '[Brave Settings Overrides] Could not find toggle id on cookies page')
    } else {
      doNotTrackToggle.setAttribute('hidden', 'true')
    }
  }
})
