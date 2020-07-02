// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {RegisterStyleOverride} from 'chrome://brave-resources/polymer_overriding.js'

RegisterStyleOverride(
  'settings-menu',
  html`
    <style>
      :host {
        --brave-settings-menu-margin-v: 30px;
        --brave-settings-menu-padding: 30px;
        --settings-nav-item-color: #424242 !important;
        position: sticky;
        top: var(--brave-settings-menu-margin-v);
        margin: 0 var(--brave-settings-menu-margin) !important;
        max-height: calc(100vh - 56px - (var(--brave-settings-menu-margin-v) * 2) - (var(--brave-settings-menu-padding) * 2));
        min-width: 172px;
        border-radius: 6px;
        background-color: #fff;
        overflow-y: auto;
        padding: 30px !important;
      }

      @media (prefers-color-scheme: dark) {
        :host {
          --settings-nav-item-color: #F4F4F4 !important;
          border-color: transparent !important;
          background-color: #161719;
        }
      }

      a[href] {
        font-weight: 400 !important;
        margin: 0 20px 20px 0 !important;
        margin-inline-start: 0 !important;
        margin-inline-end: 0 !important;
        padding-bottom: 0 !important;
        padding-top: 0 !important;
        padding-inline-start: 0 !important;
      }

      a[href].iron-selected {
        color: var(--settings-nav-item-color) !important;
        font-weight: 600 !important;
      }

      iron-icon {
        color: var(--settings-nav-item-color);
        margin-inline-end: 16px !important;
        width: 20px;
        height: 20px;
      }

      @media (prefers-color-scheme: dark) {
        a[href].iron-selected iron-icon {
          color: var(--settings-nav-item-color) !important;
        }
      }

      a[href],
      #advancedButton {
        --cr-selectable-focus_-_outline: var(--brave-focus-outline) !important;
      }

      #advancedButton {
        padding: 0 !important;
        margin-top: 30px !important;
        line-height: 1.25 !important;
        border: none !important;
      }

      #advancedButton > iron-icon {
        margin-inline-end: 0 !important;
      }

      #settingsHeader,
      #advancedButton {
        align-items: center !important;
        font-weight: normal !important;
        font-size: larger !important;
        color: var(--settings-nav-item-color) !important;
        margin-bottom: 20px !important;
      }

      #about-menu {
        display: flex;
        flex-direction: row;
        align-items: flex-start;
        justify-content: flex-start;
        color: #c5c5d3 !important;
        margin: 16px 0 0 0 !important;
      }
      .brave-about-graphic {
        flex: 0;
        flex-basis: 30%;
        display: flex;
        align-items: center;
        justify-content: flex-start;
        align-self: stretch;
      }
      .brave-about-meta {
        flex: 1;
      }
      .brave-about-item {
        display: block;
      }
    </style>
  `
)