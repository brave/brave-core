// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { RegisterStyleOverride } from '//resources/brave/polymer_overriding.js'
import { html } from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { CertificateManagerElement } from './certificate_manager-chromium.js'

RegisterStyleOverride(
  'certificate-manager',
  html`
    <style>
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

      .cr-nav-menu-item cr-icon {
        display: none !important;
      }

      .cr-nav-menu-item cr-ripple {
        display: none !important;
      }
    </style>
  `
)

// Note: This is a dynamic import so the style override is registered before the
// component is defined.
export * from './certificate_manager-chromium.js'

// Note: This needs to be defined after we register the style override.
customElements.define(
    CertificateManagerElement.is, CertificateManagerElement);
