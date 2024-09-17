// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/brave/leo.bundle.js'

import {CrLitElement, css} from '//resources/lit/v3_0/lit.rollup.js';
import {getHtml} from './cr_button.html.js';

export interface CrButtonElement {
  $: {
    button: HTMLElement
  };
}

export class CrButtonElement extends CrLitElement {
  static get is() {
    return 'cr-button';
  }

  static override get styles() {
    return css`
:host {
  display: inline-block;
  height: min-content;
}

:host(.cancel-button) {
  margin-inline-end: var(--leo-spacing-m);
}

leo-button {
  display: flex;
  width: 100%;
  height: 100%;
  align-items: center;
}
`;
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
        observer: 'disabledChanged_'
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
