// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/brave/leo.bundle.js'

import { CrLitElement, css } from '//resources/lit/v3_0/lit.rollup.js';
import { getHtml } from './cr_toggle.html.js';

export const MOVE_THRESHOLD_PX: number = 5;

export interface CrToggleElement {
  $: {
    toggle: HTMLElement
    knob: HTMLElement
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
        notify: true,
      },

      disabled: {
        type: Boolean,
        reflect: true,
      },
    };
  }

  checked: boolean = false;
  disabled: boolean = false;

  override firstUpdated(){
    this.addEventListener('click', e => {
      // Prevent |click| event from bubbling. It can cause parents of this
      // elements to erroneously re-toggle this control.
      e.stopPropagation();
      e.preventDefault();
    })
  }

  // The Nala event looks a bit different to the Chromium one, so we need to
  // convert it.
  async onChange_(e: { checked: boolean }) {
    this.checked = e.checked

    // Yield, so that 'checked-changed' (originating from `notify: 'true'`) fire
    // before the 'change' event below, which guarantees that any Polymer parent
    // with 2-way bindings on the `checked` attribute are updated first.
    await this.updateComplete

    this.dispatchEvent(new CustomEvent('change', { bubbles: true, composed: true, detail: this.checked }))
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cr-toggle': CrToggleElement;
  }
}

customElements.define(CrToggleElement.is, CrToggleElement);
