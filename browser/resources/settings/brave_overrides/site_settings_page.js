// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerComponentBehaviors} from 'chrome://brave-resources/polymer_overriding.js'
import {ContentSettingsTypes} from '../site_settings/constants.js'
import {routes} from '../route.js'

const SITE_SETTINGS_REMOVE_IDS = [
  ContentSettingsTypes.ADS,
  ContentSettingsTypes.BACKGROUND_SYNC,
]

RegisterPolymerComponentBehaviors({
  'settings-site-settings-page': [{
    registered: function() {
      if (!this.properties || !this.properties.lists_ || !this.properties.lists_.value) {
        console.error('[Brave Settings Overrides] Could not find polymer lists_ property')
        return
      }
      const oldListsGetter = this.properties.lists_.value
      this.properties.lists_.value = function () {
        const lists_ = oldListsGetter()
        if (!lists_ || !lists_.all) {
          console.error('[Brave Settings Overrides] did not get lists_ data')
          return
        }
        lists_.all = lists_.all.filter(item => !SITE_SETTINGS_REMOVE_IDS.includes(item.id))
        let indexForAutoplay = lists_.all.findIndex(item => item.id === ContentSettingsTypes.AUTOMATIC_DOWNLOADS)
        if (indexForAutoplay === -1) {
          console.error('Could not find automatic downloads site settings item')
        } else {
          indexForAutoplay++
          const autoplayItem = {
            route: routes.SITE_SETTINGS_AUTOPLAY,
            id: 'autoplay',
            label: 'siteSettingsAutoplay',
            icon: 'cr:extension',
            enabledLabel: 'siteSettingsAllowed',
            disabledLabel: 'siteSettingsBlocked'
          }
          lists_.all.splice(indexForAutoplay, 0, autoplayItem)
        }
        return lists_
      }
    }
  }]
})
