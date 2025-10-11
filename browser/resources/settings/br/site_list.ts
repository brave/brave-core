// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { SiteListElement } from '../site_settings/site_list.js'
import { ContentSettingsTypes } from '../site_settings/constants.js'

import {
  RegisterPolymerComponentReplacement,
  RegisterPolymerTemplateModifications,
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'site-list': (templateContent) => {
    const allowButton = templateContent.querySelector('#allow')
    if (!allowButton) {
      console.error('[Settings] Could not find #allow button in site-list')
      return
    }
    allowButton.textContent = '[[getAllowButtonLabel_(category)]]'

    const blockButton = templateContent.querySelector('#block')
    if (!blockButton) {
      console.error('[Settings] Could not find #block button in site-list')
      return
    }
    blockButton.textContent = '[[getBlockButtonLabel_(category)]]'
  }
})

RegisterPolymerComponentReplacement(
  'site-list',
  class BraveSiteListElement extends SiteListElement {
    private getAllowButtonLabel_() {
      if (this.category === ContentSettingsTypes.BRAVE_SHIELDS) {
        return this.i18n('siteSettingsShieldsUp')
      }
      // Default
      return this.i18n('siteSettingsActionAllow')
    }

    private getBlockButtonLabel_() {
      if (this.category === ContentSettingsTypes.BRAVE_SHIELDS) {
        return this.i18n('siteSettingsShieldsDown')
      }
      // Default
      return this.i18n('siteSettingsActionBlock')
    }
  }
)
