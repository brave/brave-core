// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {BaseMixin} from '../base_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {loadTimeData} from '../i18n_setup.js'
import '../settings_shared.css.js'
import '../settings_vars.css.js'
import '../controls/settings_checkbox.js'
import {getTemplate} from './brave_clear_browsing_data_on_exit_page.html.js'

export interface SettingsBraveClearBrowsingDataOnExitPageElement {
  $: {
    checkboxes: HTMLElement,
  }
}

const SettingsBraveClearBrowsingDataOnExitPageElementBase =
  PrefsMixin(BaseMixin(PolymerElement))

export class SettingsBraveClearBrowsingDataOnExitPageElement
extends SettingsBraveClearBrowsingDataOnExitPageElementBase {
  static get is() {
    return 'settings-brave-clear-browsing-data-on-exit-page'
  }

  static get template() {
    return getTemplate()
  }

  static getProperties() {
    return {
      prefs: {
        type: Object,
        notify: true,
      },

      counters: {
        type: Object,
        // Will be filled as results are reported.
        value() {
          return {}
        }
      },

      isModified_: {
        type: Boolean,
        value: false,
      },

      isChildAccount_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('isChildAccount')
        },
      },

      isAIChatAndHistoryAllowed_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('isLeoAssistantAllowed')
              && loadTimeData.getBoolean('isLeoAssistantHistoryAllowed')
        },
      }
    }
  }

  public isModified_: boolean

  private counters: {[k: string]: string} = {}
  private isChildAccount_: boolean
  private isAIChatAndHistoryAllowed_: boolean

  override ready() {
    super.ready()
    this.addEventListener(
      'settings-boolean-control-change', this.updateModified_)
  }

  public setCounter(counter: string, text: string) {
    this.set('counters.' + counter, text)
  }

  public getChangedSettings() {
    let changed: Array<{key: string, value: boolean}> = []
    const boxes = this.$.checkboxes.querySelectorAll('settings-checkbox')
    boxes.forEach((checkbox) => {
      if (checkbox.checked !== this.get(checkbox.pref!.key, this.prefs).value) {
        changed.push({key:checkbox.pref!.key, value:checkbox.checked})
      }
    })
    return changed
  }

  private updateModified_() {
    let modified = false
    const boxes = this.$.checkboxes.querySelectorAll('settings-checkbox')
    for (let checkbox of boxes) {
      if (checkbox.checked !== this.get(checkbox.pref!.key, this.prefs).value) {
        modified = true
        break
      }
    }

    if (this.isModified_ !== modified) {
      this.isModified_ = modified
      this.fire('clear-data-on-exit-page-change')
    }
  }
}

customElements.define(
  SettingsBraveClearBrowsingDataOnExitPageElement.is,
  SettingsBraveClearBrowsingDataOnExitPageElement)
