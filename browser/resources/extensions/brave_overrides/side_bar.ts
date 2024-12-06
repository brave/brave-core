// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import { ExtensionsSidebarElement } from '../sidebar.js'

injectStyle(ExtensionsSidebarElement, css`
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

  .cr-nav-menu-item iron-icon {
    display: none !important;
  }

  .cr-nav-menu-item cr-ripple {
    display: none !important;
  }

  .separator {
    display: none !important;
  }

  #moreExtensions {
    display: none !important;
  }
`
)
