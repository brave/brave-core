// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from '//resources/lit/v3_0/lit.rollup.js';

import type { CrButtonElement } from "./cr_button.ts"

export function getHtml(this: CrButtonElement) {
  return html`
<style>
  :host(.cancel-button) {
    margin-inline-end: var(--leo-spacing-m);
  }

  leo-button {
    display: flex;
    width: 100%;
    height: 100%;
  }
</style>
<leo-button id="button" isDisabled="${this.disabled}" kind="outline" size="small">
  <slot slot="icon-before" name="prefix-icon"></slot>
  <slot></slot>
  <slot slot="icon-after" name="suffix-icon"></slot>
</leo-button>`
}

