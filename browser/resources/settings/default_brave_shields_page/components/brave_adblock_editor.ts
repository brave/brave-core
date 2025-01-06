/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {BaseMixin} from '../../base_mixin.js'

import {getTemplate} from './brave_adblock_editor.html.js'

const AdBlockFiltersEditorBase = PrefsMixin(I18nMixin(BaseMixin(PolymerElement)))

class AdBlockFiltersEditor extends AdBlockFiltersEditorBase {
  static get is() {
    return 'adblock-filter-editor'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      value: {
        type: String
      }
    }
  }

  private value: string

  override ready() {
    super.ready()
  }

  handleInputChange_(e: Event) {
    this.value = (e.target as HTMLInputElement).value
  }

  handleSave_() {
    this.fire('save', { value: this.value })
  }
}

customElements.define(AdBlockFiltersEditor.is, AdBlockFiltersEditor)
