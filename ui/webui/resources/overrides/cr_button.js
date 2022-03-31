// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

export default html`
  <style>
    /* Default/light theme button colors */
    :host(:not([style])) {
      --brave-text-color: #3b3e4f; /* grey800 */
      --brave-brand-color: #ff7654; /* orange400 */
      --brave-primary-hover: #ff977d; /* orange300 */
      --brave-default-hover: rgba(255, 118, 84, 0.1); /* orange400 */
      --brave-focus-outline: rgba(255, 118, 84, 0.4); /* orange400 */
      --ink-color: var(--brave-brand-color) !important;
      --text-color: var(--brave-text-color) !important;
      --text-color-action: white !important;
      --border-color: #c2c4cf !important; /* grey 400 */
      --active-shadow-rgb: 255,118,84 !important;
      --active-shadow-action-rgb: 255,118,84 !important;
      --bg-action: var(--brave-brand-color) !important;
      /* TODO: Disabled colors:
      --disabled-bg-action: var(--google-grey-refresh-100);
      --disabled-bg: white;
      --disabled-border-color: var(--google-grey-refresh-100); */
      --hover-bg-action: var(--brave-primary-hover) !important;
      --hover-bg-color: none !important;
      --hover-border-color: var(--brave-brand-color) !important;
      --hover-shadow-action-rgb: 255, 118, 84 !important;
      --ink-color-action: white !important;
      font-family: Muli !important;
      outline: none !important;
      border-radius: 100px !important; /* absurdly large to make sure is
                                          rounded at any height */
    }
    :host([role='menuitem']) {
      border-radius: 0px !important;
    }
    :host(:not([style]):hover) {
      --text-color: var(--brave-brand-color) !important;
      --border-color: var(--text-color) !important;
    }
    /* Dark theme colors */
    @media (prefers-color-scheme: dark) {
      :host(:not([style])) {
        --brave-text-color: #ffffff;
        --border-color: #5e6175 !important; /* grey 700 */
      }
    }
    :host-context(.focus-outline-visible):host(:not([style]):focus) {
      box-shadow: 0 0 0 3px var(--brave-focus-outline) !important;
    }
  </style>
`
