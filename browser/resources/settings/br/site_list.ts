// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { SiteListElement } from '../site_settings/site_list.js'
import { ContentSettingsTypes } from '../site_settings/constants.js'

// The lit_mangler override of this element's template binds to these.
declare module '../site_settings/site_list.js' {
  interface SiteListElement {
    getAllowButtonLabel_(): string
    getBlockButtonLabel_(): string
  }
}

SiteListElement.prototype.getAllowButtonLabel_ = function(
    this: SiteListElement) {
  if (this.category === ContentSettingsTypes.BRAVE_SHIELDS) {
    return this.i18n('siteSettingsShieldsUp')
  }
  return this.i18n('siteSettingsActionAllow')
}

SiteListElement.prototype.getBlockButtonLabel_ = function(
    this: SiteListElement) {
  if (this.category === ContentSettingsTypes.BRAVE_SHIELDS) {
    return this.i18n('siteSettingsShieldsDown')
  }
  return this.i18n('siteSettingsActionBlock')
}
