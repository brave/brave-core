/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */


/**
 * @fileoverview `sync-url-input` is a single-line text field that is used
 * for setting up custom sync server url. It is based on
 * `home-url-input`.
 */
import 'chrome://resources/cr_elements/cr_textarea/cr_textarea.js';
import '/shared/settings/prefs/prefs.js';

import type { CrInputElement } from 'chrome://resources/cr_elements/cr_input/cr_input.js';
import { assert } from 'chrome://resources/js/assert.js';
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { PrefControlMixin } from '/shared/settings/controls/pref_control_mixin.js';
import { PrefsMixin, PrefsMixinInterface } from '/shared/settings/prefs/prefs_mixin.js';

import { BraveSyncBrowserProxy } from './brave_sync_browser_proxy.js';

import { getTemplate } from './sync_url_input.html.js';

export interface SyncUrlInputElement {
  $: {
    input: CrInputElement,
  };
}
const SyncUrlInputElementBase =
  PrefsMixin(PrefControlMixin(PolymerElement)) as
  { new(): PolymerElement & PrefsMixinInterface };


export class SyncUrlInputElement extends SyncUrlInputElementBase {
  static get is() {
    return 'sync-url-input';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      /*
       * The value of the input field.
       */
      value: String,

      /**
       * The preference object to control.
       */
      pref: { observer: 'prefChanged_' },


      /**
       * Indicates whether the input is invalid.s
       */
      invalid: {type: Boolean, value: false},
    };
  }

  value: string;
  pref: chrome.settingsPrivate.PrefObject<string> | undefined;
  invalid: boolean;

  private browserProxy_: BraveSyncBrowserProxy =
    BraveSyncBrowserProxy.getInstance();

  private onKeyPress_(e: KeyboardEvent) {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      this.validate_();
    }
  }

  private validate_() {
    if (this.value === '') {
      this.invalid = false;
      return;
    }

    this.browserProxy_.validateCustomSyncUrl(this.value).then(isValid => {
      this.invalid = !isValid;
    });
  }

  /**
   * Polymer changed observer for |pref|.
   */
  private prefChanged_() {
    if (!this.pref) {
      return;
    }

    this.setInputValueFromPref_();
  }

  private setInputValueFromPref_() {
    assert(this.pref!.type === chrome.settingsPrivate.PrefType.URL);
    this.value = this.pref!.value;
  }

  private onChange_() {
    if (this.invalid) {
      this.resetValue_();
      return;
    }

    assert(this.pref!.type === chrome.settingsPrivate.PrefType.URL);
    this.set('pref.value', this.value);
  }

  private resetValue_() {
    this.invalid = false;
    this.setInputValueFromPref_();
    this.$.input.blur();
  }

  /**
   * Focus the custom input field.
   */
  override focus() {
    this.$.input.focusInput();
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'sync-url-input': SyncUrlInputElement;
  }
}

customElements.define(SyncUrlInputElement.is, SyncUrlInputElement);
