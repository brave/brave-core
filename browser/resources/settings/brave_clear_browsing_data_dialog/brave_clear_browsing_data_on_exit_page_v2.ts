/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { PrefsMixin } from '/shared/settings/prefs/prefs_mixin.js'
import { BaseMixin } from '../base_mixin.js'
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { loadTimeData } from '../i18n_setup.js'
import { CrSettingsPrefs } from '/shared/settings/prefs/prefs_types.js';

import '../settings_shared.css.js'
import '../settings_vars.css.js'
import '../controls/settings_checkbox.js'
import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_icon/cr_icon.js'
import '../icons.html.js'

import { getTemplate } from './brave_clear_browsing_data_on_exit_page_v2.html.js'
import { BrowsingDataType } from '../clear_browsing_data_dialog/clear_browsing_data_browser_proxy.js'
import { ALL_BROWSING_DATATYPES_LIST, BrowsingDataTypeOption, getDataTypeLabel, getDataTypePrefName } from '../clear_browsing_data_dialog/clear_browsing_data_dialog_v2.js'

/**
 * Returns the pref name for clearing data on exit.
 * Reuses Chromium's getDataTypePrefName and appends '_on_exit' suffix.
 */
function getDataTypePrefNameOnExit(datatypes: BrowsingDataType) {
  return getDataTypePrefName(datatypes) + '_on_exit';
}

export interface SettingsBraveClearBrowsingDataOnExitPageV2Element {
  $: {
    checkboxContainer: HTMLElement,
  }
}

const SettingsBraveClearBrowsingDataOnExitPageV2ElementBase =
  PrefsMixin(BaseMixin(PolymerElement))

export class SettingsBraveClearBrowsingDataOnExitPageV2Element
  extends SettingsBraveClearBrowsingDataOnExitPageV2ElementBase {
  static get is() {
    return 'settings-brave-clear-browsing-data-on-exit-page-v2'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      prefs: {
        type: Object,
        notify: true,
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

      browsingDataTypeOptionsList_: {
        type: Array,
        value: [],
      },

      // <if expr="enable_ai_chat">
      isLeoAssistantAndHistoryAllowed_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('isLeoAssistantAllowed')
            && loadTimeData.getBoolean('isLeoAssistantHistoryAllowed')
        },
      }
      // </if>
    }
  }

  declare prefs: any
  declare isModified_: boolean
  declare private browsingDataTypeOptionsList_: BrowsingDataTypeOption[]

  declare private isChildAccount_: boolean
  // <if expr="enable_ai_chat">
  declare private isLeoAssistantAndHistoryAllowed_: boolean
  // </if>

  override ready() {
    super.ready()

    CrSettingsPrefs.initialized.then(() => {
      this.setUpDataTypeOptionLists_();
    });

    this.addEventListener(
      'settings-boolean-control-change', this.updateModified_)
  }

  public getChangedSettings() {
    let changed: Array<{ key: string, value: boolean }> = []
    const checkboxContainer = this.$.checkboxContainer
    const boxes = checkboxContainer.querySelectorAll('settings-checkbox')
    boxes.forEach((checkbox) => {
      if (checkbox.checked !== this.get(checkbox.pref!.key, this.prefs).value) {
        changed.push({ key: checkbox.pref!.key, value: checkbox.checked })
      }
    })
    return changed
  }

  /**
   * Sets up the data type options list
   */
  private setUpDataTypeOptionLists_() {
    const optionsList: BrowsingDataTypeOption[] = [];

    ALL_BROWSING_DATATYPES_LIST.forEach((datatype: BrowsingDataType) => {
      if (!this.shouldDataTypeBeIncluded_(datatype)) {
        return;
      }

      const datatypeOption: BrowsingDataTypeOption = {
        label: getDataTypeLabel(datatype),
        pref: this.getPref(getDataTypePrefNameOnExit(datatype)),
      };

      optionsList.push(datatypeOption);
    });

    this.browsingDataTypeOptionsList_ = optionsList;
  }

  /**
   * Determines if a data type should be included in the lists
   */
  private shouldDataTypeBeIncluded_(datatype: BrowsingDataType): boolean {
    // Skip browsing/download history for child accounts
    if (this.isChildAccount_ &&
      (datatype === BrowsingDataType.HISTORY ||
        datatype === BrowsingDataType.DOWNLOADS)) {
      return false;
    }

    if (datatype === BrowsingDataType.BRAVE_AI_CHAT) {
      // <if expr="enable_ai_chat">
      if (!this.isLeoAssistantAndHistoryAllowed_) {
        return false;
      }
      // </if>

      // <if expr="not enable_ai_chat">
      return false;
      // </if>
    }

    return true;
  }

  private updateModified_() {
    let modified = false
    const checkboxContainer = this.$.checkboxContainer
    const boxes = checkboxContainer.querySelectorAll('settings-checkbox')
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
  SettingsBraveClearBrowsingDataOnExitPageV2Element.is,
  SettingsBraveClearBrowsingDataOnExitPageV2Element)
