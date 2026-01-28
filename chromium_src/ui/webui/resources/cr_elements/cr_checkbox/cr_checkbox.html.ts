// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {html, nothing} from '//resources/lit/v3_0/lit.rollup.js';

import type {CrCheckboxElement} from './cr_checkbox.js';

export function getHtml(this: CrCheckboxElement) {
  return html`
<div id="checkbox" tabindex="${this.tabIndex}" role="checkbox"
    @keydown="${this.onKeyDown_}" @keyup="${this.onKeyUp_}"
    aria-disabled="${this.getAriaDisabled_()}"
    aria-checked="${this.getAriaChecked_()}"
    aria-label="${this.ariaLabelOverride || nothing}"
    aria-labelledby="${this.ariaLabelOverride ? nothing : 'labelContainer'}"
    aria-describedby="ariaDescription">
  <!-- Leo unchecked icon from checkbox-unchecked.svg -->
  <svg id="uncheckmark" width="16" height="16" viewBox="2 2 20 20"
      fill="none" xmlns="http://www.w3.org/2000/svg">
    <path fill-rule="evenodd" clip-rule="evenodd"
        d="M13.003 4.1h-2.006c-1.53 0-2.579.001-3.392.069-.793.066-1.217.186-1.523.345a3.7 3.7 0 0 0-1.568 1.568c-.159.306-.28.73-.345 1.523-.068.813-.069 1.863-.069 3.392v2.006c0 1.53.001 2.579.069 3.392.066.793.186 1.217.345 1.523a3.7 3.7 0 0 0 1.568 1.568c.306.159.73.28 1.523.345.813.068 1.863.069 3.392.069h2.006c1.53 0 2.579-.001 3.392-.069.793-.066 1.217-.186 1.523-.345a3.7 3.7 0 0 0 1.568-1.568c.159-.306.28-.73.345-1.523.068-.813.069-1.862.069-3.392v-2.006c0-1.53-.001-2.579-.069-3.392-.066-.793-.186-1.217-.345-1.523a3.7 3.7 0 0 0-1.568-1.568c-.306-.159-.73-.28-1.523-.345-.813-.068-1.862-.069-3.392-.069M3.094 5.345C2.5 6.489 2.5 7.992 2.5 10.997v2.006c0 3.005 0 4.508.594 5.652.5.964 1.287 1.75 2.25 2.25 1.145.595 2.648.595 5.653.595h2.006c3.005 0 4.508 0 5.652-.594a5.3 5.3 0 0 0 2.25-2.25c.595-1.145.595-2.648.595-5.653v-2.006c0-3.005 0-4.508-.594-5.652a5.3 5.3 0 0 0-2.25-2.25C17.51 2.5 16.007 2.5 13.002 2.5h-2.006c-3.005 0-4.508 0-5.652.594a5.3 5.3 0 0 0-2.25 2.25">
  </svg>
  <!-- Leo checked icon from checkbox-checked.svg -->
  <svg id="checkmark" width="16" height="16" viewBox="2 2 20 20"
      fill="none" xmlns="http://www.w3.org/2000/svg">
    <path fill-rule="evenodd" clip-rule="evenodd"
        d="M3.094 5.345C2.5 6.489 2.5 7.992 2.5 10.997v2.006c0 3.005 0 4.508.594 5.652.5.964 1.287 1.75 2.25 2.25 1.145.595 2.648.595 5.653.595h2.006c3.005 0 4.508 0 5.652-.594a5.3 5.3 0 0 0 2.25-2.25c.595-1.145.595-2.648.595-5.653v-2.006c0-3.005 0-4.508-.594-5.652a5.3 5.3 0 0 0-2.25-2.25C17.51 2.5 16.007 2.5 13.002 2.5h-2.006c-3.005 0-4.508 0-5.652.594a5.3 5.3 0 0 0-2.25 2.25m13.088 4.726c.334-.363.338-.956.009-1.324a.796.796 0 0 0-1.202-.01l-4.186 4.546-1.794-1.933a.796.796 0 0 0-1.202.015c-.328.37-.322.963.013 1.324l2.39 2.576a.795.795 0 0 0 1.191-.002z">
  </svg>
  <div id="hover-layer"></div>
</div>
<div id="labelContainer" part="label-container">
  <slot></slot>
</div>
<div id="ariaDescription" aria-hidden="true">${this.ariaDescription}</div>`;
}
