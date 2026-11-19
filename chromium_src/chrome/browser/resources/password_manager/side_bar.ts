// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_icon/cr_icon.js'
import 'chrome://resources/cr_elements/cr_menu_selector/cr_menu_selector.js'
import 'chrome://resources/cr_elements/cr_ripple/cr_ripple.js'

import {injectStyle} from '//resources/brave/lit_overriding.js'
import {css} from '//resources/lit/v3_0/lit.rollup.js'

import {PasswordManagerSideBarElement} from './side_bar-chromium.js'

injectStyle(PasswordManagerSideBarElement, css`
  .cr-nav-menu-item {
    min-height: 20px !important;
    border-end-end-radius: 0px !important;
    border-start-end-radius: 0px !important;
    box-sizing: content-box !important;
  }

  .cr-nav-menu-item:hover {
    background: transparent !important;
  }

  .cr-nav-menu-item[selected] {
    --iron-icon-fill-color: var(--cr-link-color) !important;
    color: var(--cr-link-color) !important;
    background: transparent !important;
  }

  .cr-nav-menu-item cr-ripple {
    display: none !important;
  }
`)

export * from './side_bar-chromium.js'
