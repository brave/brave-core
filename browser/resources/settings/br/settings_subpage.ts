// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {injectStyle} from '//resources/brave/lit_overriding.js'
import {css} from '//resources/lit/v3_0/lit.rollup.js'

import {SettingsSubpageElement} from '../settings_page/settings_subpage.js'

injectStyle(SettingsSubpageElement, css`
  :host {
    min-height: auto !important;
    display: inline;
    position: relative;
  }
  :host(:not(.multi-card)) {
    background-color: var(--leo-color-page-background) !important;
    box-shadow: none !important;
  }
  :host(:not(.multi-card):not([id=safetyHub])) slot {
    box-shadow: var(--leo-effect-elevation-01) !important;
    background-color: var(--leo-color-container-background) !important;
    border-radius: var(--leo-radius-m) !important;
    display: block;
    padding-bottom: var(--leo-spacing-2xl);
  }
  #headerLine {
    padding-top: 0 !important;
    padding-left: var(--leo-spacing-m) !important;
    padding-right: var(--leo-spacing-m) !important;
    padding-bottom: 0 !important;
    margin-top: var(--cr-section-vertical-margin);
  }
  .cr-title-text {
    color: var(--cr-title-text-color);
    font-weight: 600 !important;
    font-size: var(--leo-typography-heading-h4-font-size) !important;
  }
`)
