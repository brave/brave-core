/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {injectStyle} from '//resources/brave/lit_overriding.js'
import {css} from '//resources/lit/v3_0/lit.rollup.js'

import {SettingsMenuElement} from '../settings_menu/settings_menu.js'
import 'chrome://resources/brave/leo.bundle.js'

injectStyle(SettingsMenuElement, css`
  :host {
    --brave-settings-menu-margin-v: 24px;
    --brave-settings-menu-padding: 24px;
    --settings-nav-item-color: var(--leo-color-text-secondary) !important;
    position: sticky;
    top: var(--brave-settings-menu-margin-v);
    margin: 0 !important;
    min-width: 172px;
    overflow-y: auto;
    padding: 12px 24px !important;
  }
  .cr-nav-menu-item {
    min-height: 20px !important;
    border-end-end-radius: 0px !important;
    border-start-end-radius: 0px !important;
    box-sizing: content-box !important;
    overflow: visible !important;

    --iron-icon-width: 20px;
    --iron-icon-height: 20px;
    --iron-icon-fill-color: currentColor;
  }

  .cr-nav-menu-item:hover {
    background: transparent !important;
    --iron-icon-fill-color: var(--leo-color-icon-interactive);
  }

  .cr-nav-menu-item[selected] {
    --iron-icon-fill-color: var(--leo-color-icon-interactive);

    color: var(--leo-color-text-interactive) !important;
    background: transparent !important;
  }

  .cr-nav-menu-item:focus {
    outline: none !important;
  }

  .cr-nav-menu-item cr-ripple {
    display: none !important;
  }

  .menu-separator {
    margin: 4px -24px !important;
  }

  @media (prefers-color-scheme: dark) {
    :host {
      --settings-nav-item-color: var(--leo-color-text-primary) !important;
      border-color: transparent !important;
    }
  }

  a[href] {
    font-weight: 500 !important;
    margin: 0 20px 22px 0 !important;
    margin-inline-start: 0 !important;
    margin-inline-end: 0 !important;
    padding-bottom: 0 !important;
    padding-top: 0 !important;
    padding-inline-start: 0 !important;
    position: relative !important;
  }

  a[href]:focus-visible {
    box-shadow: 0 0 0 4px rgba(160, 165, 235, 1) !important;
    outline: none !important;
    border-radius: 6px !important;
  }

  a[href].selected {
    color: #DB2F04;
  }

  a:hover, cr-icon:hover {
    color: var(--leo-color-icon-interactive) !important;
  }

  cr-icon, leo-icon {
    margin-inline-end: 16px !important;
    width: 20px;
    height: 20px;
  }

  a[href].selected::before {
    content: "";
    position: absolute;
    top: 50%;
    left: calc(-1 * var(--brave-settings-menu-padding));
    transform: translateY(-50%);
    display: block;
    height: 28px;
    width: 4px;
    background: var(--leo-color-text-interactive);
    border-radius: 0px 2px 2px 0px;
  }

  @media (prefers-color-scheme: dark) {
    a[href].selected {
      color: #FB5930;
    }

    a:hover, cr-icon:hover {
      --iron-icon-fill-color: var(--leo-color-icon-interactive) !important;
      color: var(--leo-color-icon-interactive) !important;
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

  #advancedButton > cr-icon {
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

  #autofill {
    margin-top: 20px !important;
  }

  #about-menu {
    display: flex;
    flex-direction: row;
    align-items: flex-start;
    justify-content: flex-start;
    color: var(--leo-color-text-tertiary) !important;
    margin: 16px 0 0 0 !important;
    text-decoration: none !important;
  }
  .brave-about-graphic {
    flex: 0;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    align-self: stretch;
    margin-right: var(--leo-spacing-xl);
  }
  .brave-about-menu-link-text{
    font-size: 14px !important;
    font-weight: 500 !important;
    color: var(--leo-color-text-secondary) !important;
  }
  .brave-about-meta {
    flex: 1;
  }
  .brave-about-item {
    display: block;
  }
`)
