// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/brave/leo.bundle.js'

import { PolymerElement } from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { getTemplate } from './cr_toggle.html.js';

export interface CrToggleElement {
  $: {
    toggle: HTMLElement
  }
}

export class CrToggleElement extends PolymerElement {
  static get is() {
    return 'cr-toggle';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      checked: {
        type: Boolean,
        value: false,
        reflectToAttribute: true,
        notify: true,
      },

      disabled: {
        type: Boolean,
        value: false,
        reflectToAttribute: true,
      },
    };
  }

  checked: boolean;
  disabled: boolean;

  // The Nala event looks a bit different to the Chromium one, so we need to
  // convert it.
  private onChange_(e: { checked: boolean }) {
    this.checked = e.checked
    this.dispatchEvent(new CustomEvent('change', { detail: e.checked }))
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cr-toggle': CrToggleElement;
  }
}

customElements.define(CrToggleElement.is, CrToggleElement);
