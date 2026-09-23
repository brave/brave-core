// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { html } from '//resources/lit/v3_0/lit.rollup.js';
import type { CrToggleElement } from './cr_toggle.js';

export function getHtml(this: CrToggleElement) {
  return html`
<leo-toggle id="toggle" size="small" ?checked="${this.checked}" ?disabled="${this.disabled}" @change="${this.onChange_}">
  <slot></slot>
</leo-toggle>
`
}
