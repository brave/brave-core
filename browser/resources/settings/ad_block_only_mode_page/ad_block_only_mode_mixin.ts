// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {dedupingMixin} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {loadTimeData} from '../i18n_setup.js'

const AD_BLOCK_ONLY_MODE_ENABLED_PREF = 'brave.shields.adblock_only_mode_enabled'

type Constructor<T> = new (...args: any[]) => T;

export interface AdBlockOnlyModeMixinInterface {
  setAdBlockOnlyModeEnabled(enabled: boolean): void;
  isAdBlockOnlyModeEnabled_: boolean;
}

export const AdBlockOnlyModeMixin = dedupingMixin(
    <T extends Constructor<PolymerElement>>(superClass: T): T&
    Constructor<AdBlockOnlyModeMixinInterface> => {
      class AdBlockOnlyModeMixin extends superClass implements
      AdBlockOnlyModeMixinInterface {
        static get properties() {
          return {
            isAdBlockOnlyModeEnabled_: {
              type: Boolean,
              value: false,
            },
          }
        }

        declare isAdBlockOnlyModeEnabled_: boolean

        override ready() {
          super.ready()

          chrome.settingsPrivate.getPref(AD_BLOCK_ONLY_MODE_ENABLED_PREF)
              .then((pref: chrome.settingsPrivate.PrefObject) =>
                this.onAdBlockOnlyModeEnabledPrefChanged(pref))

          chrome.settingsPrivate.onPrefsChanged.addListener(
              (prefs: chrome.settingsPrivate.PrefObject[]) => {
                const pref = prefs.find(p => p.key === AD_BLOCK_ONLY_MODE_ENABLED_PREF)
                if (pref) {
                  this.onAdBlockOnlyModeEnabledPrefChanged(pref)
                }
              })
        }

        onAdBlockOnlyModeEnabledPrefChanged(pref: chrome.settingsPrivate.PrefObject): void {
          this.isAdBlockOnlyModeEnabled_ =
              pref.value &&
              loadTimeData.getBoolean('isAdBlockOnlyModeSupportedAndFeatureEnabled')
          }

        setAdBlockOnlyModeEnabled(enabled: boolean): void {
          chrome.settingsPrivate.setPref(AD_BLOCK_ONLY_MODE_ENABLED_PREF, enabled);
        }
      }

      return AdBlockOnlyModeMixin
    })
