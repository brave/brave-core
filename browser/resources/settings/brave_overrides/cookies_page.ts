// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  html,
  RegisterPolymerComponentBehaviors,
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'

import { loadTimeData } from '../i18n_setup.js'

import { SettingsCookiesPageElement } from '../privacy_page/cookies_page.js'

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
    const additionalProtections = templateContent.
      getElementById('additionalProtections')
    if (!additionalProtections) {
      console.error(
        '[Brave Settings Overrides] Could not find additionalProtections ' +
        'id on cookies page.')
    } else {
      additionalProtections.setAttribute('hidden', 'true')
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

const BraveSettingsCookiePageBehavior = {
  ready: function (this: SettingsCookiesPageElement) {
    const siteList = this.shadowRoot!.getElementById('allow3pcExceptionsList')
    if (!siteList) {
      throw new Error(
        '[Brave Settings Overrides] Could not find allow3pcExceptionsList'
      )
    }
    const listHeader = siteList.shadowRoot!.getElementById('listHeader')
    if (!listHeader) {
      throw new Error(
        '[Brave Settings Overrides] Could not find allow3pcExceptionsList'
      )
    }
    const wrapper = document.createElement('div')
    listHeader.parentNode!.insertBefore(wrapper, listHeader)
    wrapper.appendChild(listHeader)
    wrapper.appendChild(
      html`<b>${loadTimeData.getString('cookieControlledByShieldsHeader')}</b>`
    )
  }
}

RegisterPolymerComponentBehaviors({
  'settings-cookies-page': [BraveSettingsCookiePageBehavior]
})
