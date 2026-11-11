/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { html } from 'chrome://resources/brave/polymer_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'

import { loadTimeData } from '../i18n_setup.js'

import { SettingsCookiesPageElement } from '../privacy_page/cookies_page.js'

injectStyle(
  SettingsCookiesPageElement,
  css`
    #generalControls,
    #additionalProtections,
    #siteDataTrigger,
    #doNotTrack {
      display: none;
    }
  `,
)

// `firstUpdated` is `protected` on ReactiveElement, so reach it through an
// untyped view of the prototype to patch it from outside the class hierarchy.
const proto = SettingsCookiesPageElement.prototype as unknown as {
  firstUpdated?: (changedProperties: PropertyValues) => void
}

const originalFirstUpdated = proto.firstUpdated
proto.firstUpdated = function (
  this: SettingsCookiesPageElement,
  changedProperties: PropertyValues,
) {
  originalFirstUpdated?.call(this, changedProperties)

  const siteList = this.shadowRoot.getElementById('allow3pcExceptionsList')
  if (!siteList) {
    throw new Error(
      '[Brave Settings Overrides] Could not find allow3pcExceptionsList'
    )
  }
  const listHeader = siteList.shadowRoot!.getElementById('listHeader')
  if (!listHeader) {
    throw new Error(
      '[Brave Settings Overrides] Could not find listHeader'
    )
  }
  const wrapper = document.createElement('div')
  listHeader.parentNode!.insertBefore(wrapper, listHeader)
  wrapper.appendChild(listHeader)
  wrapper.appendChild(
    html`<b>${loadTimeData.getString('cookieControlledByShieldsHeader')}</b>`
  )
}
