// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/brave/leo.bundle.js'

import {CrLitElement, css} from '//resources/lit/v3_0/lit.rollup.js';
import { getHtml } from './cr_toggle.html.js';

export interface CrToggleElement {
  $: {
    toggle: HTMLElement
  }
}

export class CrToggleElement extends CrLitElement {
  static get is() {
    return 'cr-toggle';
  }

  static override get styles() {
    return css``
  }

  override render() {
    return getHtml.bind(this)();
  }

  static override get properties() {
    return {
      checked: {
        type: Boolean,
        reflect: true,
      },

      disabled: {
        type: Boolean,
        reflect: true,
      },
    };
  }

  checked: boolean = false;
  disabled: boolean = false;

  // The Nala event looks a bit different to the Chromium one, so we need to
  // convert it.
  onChange_(e: { checked: boolean }) {
    this.checked = e.checked
    this.dispatchEvent(new CustomEvent('change', { bubbles: true, composed: true, detail: e.checked }))
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cr-toggle': CrToggleElement;
  }
}

customElements.define(CrToggleElement.is, CrToggleElement);
