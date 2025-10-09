// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { SiteListElement } from '../site_settings/site_list.js'
import { ContentSettingsTypes } from '../site_settings/constants.js'

import {
  RegisterPolymerComponentReplacement,
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerComponentReplacement(
  'site-list',
  class BraveSiteListElement extends SiteListElement {
    override ready() {
      super.ready()

      // onShowActionMenu_ is private in the superclass, so we have to
      // replace it to change its functionality.
      // This is similar to search_page.ts.
      const anyThis = this as any
      const originalOnShowActionMenu = anyThis.onShowActionMenu_.bind(this)
      anyThis.onShowActionMenu_ = (e: any) => {
        originalOnShowActionMenu(e)

        if (this.category === ContentSettingsTypes.BRAVE_SHIELDS) {
          // Update labels after menu is shown
          requestAnimationFrame(() => {
            const actionMenu = this.shadowRoot?.querySelector('cr-action-menu')
            if (!actionMenu) {
              return
            }

            const allowButton = actionMenu.querySelector('#allow')
            if (allowButton) {
              allowButton.textContent = this.i18n('siteSettingsShieldsUp')
            }

            const blockButton = actionMenu.querySelector('#block')
            if (blockButton) {
              blockButton.textContent = this.i18n('siteSettingsShieldsDown')
            }
          })
        }
      }
    }
  }
)
