// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'

import { SiteListEntryElement } from '../site_settings/site_list_entry.js'

// The lit_mangler override of this element's template binds to these.
declare module '../site_settings/site_list_entry.js' {
  interface SiteListEntryElement {
    shouldDisableReset_(): boolean
    onResetSiteMouseenter_(): void
  }
}

injectStyle(SiteListEntryElement, css`
  #resetSite[disabled] {
    pointer-events: all;
    cursor: not-allowed;
  }
`)

enum BraveCookieTypes {
  SHIELDS_DOWN = 'shields down',
  SHIELDS_SETTINGS = 'shields settings',
  GOOGLE_SIGN_IN = 'google sign-in'
}

SiteListEntryElement.prototype.shouldDisableReset_ = function(
    this: SiteListEntryElement) {
  return this.model.braveCookieType !== undefined
}

SiteListEntryElement.prototype.onResetSiteMouseenter_ = function(
    this: SiteListEntryElement) {
  const button = this.shadowRoot.querySelector('#resetSite')

  if (
    this.model.braveCookieType === BraveCookieTypes.SHIELDS_SETTINGS ||
    this.model.braveCookieType === BraveCookieTypes.SHIELDS_DOWN
  ) {
    const text = this.i18n('cookieControlledByShieldsTooltip')
    this.fire('show-tooltip', { target: button, text })
  } else if (
    this.model.braveCookieType === BraveCookieTypes.GOOGLE_SIGN_IN
  ) {
    const text = this.i18n('cookieControlledByGoogleSigninTooltip')
    this.fire('show-tooltip', { target: button, text })
  }
}
