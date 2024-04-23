// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/brave/leo.bundle.js'

import {getHtml} from './cr_button.html.js';
import {getCss} from './cr_button.css';
import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js';
import type {PropertyValues} from '//resources/lit/v3_0/lit.rollup.js';

export interface CrButtonElement {
  $: {
    button: HTMLElement,
    prefixIcon: HTMLSlotElement,
    suffixIcon: HTMLSlotElement,
  };
}

export class CrButtonElement extends CrLitElement {
  static get is() {
    return 'cr-button';
  }

  static override get styles() {
    return getCss();
  }

  override render() {
    return getHtml.bind(this)();
  }

  static override get properties() {
    return {
      disabled: {
        type: Boolean,
        value: false,
        reflect: true,
      },
      class: {
        type: String,
        value: '',
        reflect: true,
        observer: 'classChanged_'
      }
    };
  }

  disabled: boolean;
  class: string;

  override updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties);

    if (changedProperties.has('class')) {
      this.classChanged_()
    }
  }

  private onClick_(e: Event) {
    if (this.disabled) {
      e.stopImmediatePropagation();
    }
  }

  classChanged_() {
    let kind = 'outline'
    if (this.classList.contains('action-button')) {
      kind = 'filled'
    }

    // Avatar buttons should not have a border
    if (this.classList.contains('avatar') || this.classList.contains('plain')) {
      kind = 'plain'
    }

    this.$.button.setAttribute('kind', kind)
  }
}

// declare global {
//   interface HTMLElementTagNameMap {
//     'cr-button': CrButtonElement;
//   }
// }

customElements.define(CrButtonElement.is, CrButtonElement);
