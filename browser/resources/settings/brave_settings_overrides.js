// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerComponentBehaviors,
  OverrideIronIcons
} from 'chrome://brave-resources/polymer_overriding.js'

import './i18n_setup.js'

// Anything which modifies a module's export and needs to be called before
// anything else is imported, but note that we cannot predict the order that
// modules are executed in. Plus, it will be different between non-optimized
// (the browser controls which es module is downloaded and executed) and
// optimized (rollup controls the order in which modules are exectuted).
import './brave_page_visibility.js';

import './style_overrides/index.js'
import './template_overrides/index.js'

import './icons.m.js'
import './brave_icons.m.js'

import {ContentSettingsTypes} from './site_settings/constants.js'
import {routes} from './route.js'
import {
  BraveClearBrowsingDataOnExitBehavior
} from './brave_clear_browsing_data_dialog/brave_clear_browsing_data_dialog_behavior.js'
import { BraveResetProfileDialogBehavior } from './brave_reset_page/brave_reset_profile_dialog_behavior.js'

// TODO: move throttle utility to a module
function throttle (callback, maxWaitTime = 30) {
  // Call on first invocation
  let shouldWait = false;
  return function (...args) {
    if (!shouldWait) {
      callback.apply(this, args);
      shouldWait = true;
      setTimeout(function () {
        shouldWait = false;
      }, maxWaitTime);
    }
  }
}

const BraveClearSettingsMenuHighlightBehavior = {
  ready: function() {
    // Clear menu selection after scrolling away.
    // Chromium's menu is not persistant, so does not have
    // this issue.
    const container = this.$.container
    if (!container) {
      console.error('Could not find #container in settings-ui module')
    }
    const menu = this.$$('settings-menu')
    if (!menu) {
      console.error('Could not find settings-menu in settings-ui module')
    }
    let onScroll
    function stopObservingScroll() {
      if (onScroll) {
        container.removeEventListener('scroll', onScroll)
        onScroll = null
      }
    }
    window.addEventListener('showing-section', ({ detail: section }) => {
      // Currently showing or about to scroll to `section`.
      // If we're getting further away from section top
      // then section is no longer 'selected'.
      // TODO(petemill): If this wasn't a chromium module, we'd simply add a handler
      // for scrolling away, or have the menu change selection as we scroll.
      stopObservingScroll()
      function calcDistance() {
        const sectionScrollTop = section.offsetTop
        const currentScrollTop = container.scrollTop
        return Math.abs(sectionScrollTop - currentScrollTop)
      }
      let distance = calcDistance()
      onScroll = throttle(() => {
        const latestDistance = calcDistance()
        if (latestDistance > distance) {
          menu.setSelectedUrl_('')
          stopObservingScroll()
        } else {
          distance = latestDistance
        }
      }, 100)
      container.addEventListener('scroll', onScroll)
    })
  }
}

const SITE_SETTINGS_REMOVE_IDS = [
  ContentSettingsTypes.ADS,
  ContentSettingsTypes.BACKGROUND_SYNC,
]

// Polymer Component Behavior injection (like superclasses)
RegisterPolymerComponentBehaviors({
  'settings-clear-browsing-data-dialog': [
    BraveClearBrowsingDataOnExitBehavior
  ],
  'settings-reset-profile-dialog': [
    BraveResetProfileDialogBehavior
  ],
  'settings-ui': [
    BraveClearSettingsMenuHighlightBehavior
  ],
  'settings-import-data-dialog': [{
    registered: function () {
      const oldPrefsChanged = this.prefsChanged_
      if (!oldPrefsChanged) {
        console.error('[Brave Settings Overrides] cannot find prefsChanged_ on ImportDataDialog')
        return
      }
      this.prefsChanged_ = function () {
        if (typeof this.noImportDataTypeSelected_ !== 'boolean') {
          console.error('[Brave Settings Overrides] cannot find noImportDataTypeSelected_ on ImportDataDialog')
          return
        }
        oldPrefsChanged.apply(this)
        if (this.selected_ == undefined || this.prefs == undefined) {
          return;
        }
        this.noImportDataTypeSelected_ = this.noImportDataTypeSelected_ &&
          !(this.getPref('import_dialog_extensions').value &&
            this.selected_.extensions)
      }
    }
  }],
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
            enabledLabel: 'siteSettingsAutoplayAsk',
            disabledLabel: 'siteSettingsBlocked'
          }
          lists_.all.splice(indexForAutoplay, 0, autoplayItem)
        }
        return lists_
      }
    }
  }]
})

// Icons
OverrideIronIcons('settings', 'brave_settings', {
  palette: 'appearance',
  assignment: 'autofill',
  language: 'language',
  build: 'system',
  restore: 'reset-settings'
})
OverrideIronIcons('cr', 'brave_settings', {
  security: 'privacy-security',
  search: 'search-engine',
  ['file-download']: 'download',
  print: 'printing'
})
