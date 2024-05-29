// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/brave/leo.bundle.js'

import { PolymerElement } from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { getTemplate } from './cr_button.html.js';

export interface CrButtonElement {
  $: {
    button: HTMLElement
  };
}

export class CrButtonElement extends PolymerElement {
  static get is() {
    return 'cr-button';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      disabled: {
        type: Boolean,
        value: false,
        reflectToAttribute: true,
        observer: 'disabledChanged_'
      },
      class: {
        type: String,
        value: '',
        reflectToAttribute: true,
        observer: 'classChanged_'
      }
    };
  }

  disabled: boolean;
  class: string;

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
    const avatar = this.classList.contains('avatar')
    if (avatar || this.classList.contains('plain')) {
      kind = 'plain'
    }

    // Avatar buttons should be round.
    if (avatar) {
      this.$.button.setAttribute('fab', '')
    }

    this.$.button.setAttribute('kind', kind)
  }

  disabledChanged_() {
    // TODO(petemill): This should be a $= binding but the leo-button
    // has a bug with treating it as a boolean attribute
    // https://github.com/brave/leo/issues/690.
    if (this.disabled) {
      this.$.button.setAttribute('isDisabled', '_')
    } else {
      this.$.button.removeAttribute('isDisabled')
    }
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cr-button': CrButtonElement;
  }
}

customElements.define(CrButtonElement.is, CrButtonElement);
