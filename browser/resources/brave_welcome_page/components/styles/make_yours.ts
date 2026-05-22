/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'

export const makeYoursStyles = `
  /* Customize Options Styles (Make Yours step) */
  .customize-content {
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};
    width: 100%;
    max-width: 620px;
    height: 100%;
  }

  .mock-window-preview {
    position: relative;
    width: 100%;
    flex: 1;
    min-height: 0;
    border-radius: ${radius.xl};
    overflow: hidden;
  }

  .mock-window-wallpaper {
    position: absolute;
    inset: 0;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .mock-window-wallpaper-image {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: cover;
    filter: blur(6px);
    transform: scale(1.05);
    transition: opacity 0.4s ease;
  }

  .mock-window-wallpaper-image.wallpaper-hidden {
    opacity: 0;
  }

  .mock-window-wallpaper-image.wallpaper-visible {
    opacity: 1;
  }

  .mock-window-brave-icon {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    --leo-icon-size: 80px;
    z-index: 1;
  }

  /* Browser Chrome Preview Styles */
  .browser-chrome {
    position: absolute;
    inset: 32px;
    display: flex;
    flex-direction: column;
    background: ${color.container.background};
    border-radius: ${radius.l};
    overflow: hidden;
    box-shadow: 0px 0px 0px 0.75px rgba(6, 6, 5, 0.2);
    pointer-events: none;
    user-select: none;
    transition: background-color 0.4s ease;
  }

  .browser-chrome::after {
    content: '';
    position: absolute;
    inset: 0;
    pointer-events: none;
    box-shadow: inset 0px 1px 0.5px 0px rgba(255, 255, 255, 0.2);
    border-radius: inherit;
  }

  /* Tab Bar */
  .browser-tabbar {
    display: flex;
    align-items: center;
    height: 40px;
    padding: 0 4px;
    background: ${color.neutral[10]};
    transition: background-color 0.4s ease;
  }

  .browser-tabs {
    display: flex;
    flex: 1;
    align-items: center;
    gap: 4px;
  }

  .browser-tab {
    display: flex;
    align-items: center;
    gap: 8px;
    height: 32px;
    padding: 0 8px;
    border-radius: ${radius.m};
    flex: 1;
    min-width: 0;
  }

  .browser-tab leo-icon {
    --leo-icon-size: 16px;
    flex-shrink: 0;
  }

  .browser-tab.pinned {
    flex: 0 0 32px;
    width: 32px;
    padding: 6px 8px;
    justify-content: center;
    border: 1px solid ${color.divider.subtle};
  }

  .browser-tab.active {
    background: ${color.container.background};
    transition: background-color 0.4s ease;
  }

  .tab-title {
    flex: 1;
    font-size: 12px;
    line-height: 18px;
    letter-spacing: -0.08px;
    color: ${color.text.primary};
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    transition: color 0.4s ease;
  }

  .browser-tab:not(.active) .tab-title {
    color: ${color.text.secondary};
  }

  .tab-close {
    --leo-icon-size: 16px;
    flex-shrink: 0;
    color: ${color.icon.default};
    transition: color 0.4s ease;
  }

  .tab-divider {
    position: absolute;
    right: 0;
    top: 50%;
    transform: translateY(-50%);
    width: 1px;
    height: 24px;
    background: ${color.divider.subtle};
    transition: background-color 0.4s ease;
  }

  .browser-tab:not(.active):not(.pinned) {
    position: relative;
  }

  .browser-tab-add {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 28px;
    height: 28px;
    padding: 4px;
    border-radius: ${radius.m};
    margin-left: 8px;
    flex-shrink: 0;
  }

  .browser-tab-add leo-icon {
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.icon.default};
  }

  .browser-tab-dropdown {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 28px;
    height: 28px;
    padding: 4px;
    border-radius: ${radius.m};
    flex-shrink: 0;
  }

  .browser-tab-dropdown leo-icon {
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.icon.default};
  }

  /* Address Bar */
  .browser-addressbar {
    display: flex;
    align-items: center;
    gap: 16px;
    padding: 6px 8px;
    background: ${color.container.background};
    border-top-left-radius: ${radius.m};
    border-top-right-radius: 10px;
    transition: background-color 0.4s ease;
  }

  .addressbar-actions {
    display: flex;
    gap: 4px;
  }

  .toolbar-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 28px;
    height: 28px;
    padding: 4px;
    border: none;
    background: transparent;
    border-radius: ${radius.m};
    cursor: pointer;
  }

  .toolbar-btn leo-icon {
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.icon.default};
  }

  .toolbar-btn.disabled {
    opacity: 0.5;
    cursor: default;
  }

  .toolbar-btn.small {
    border-radius: ${radius.s};
  }

  .addressbar-field {
    display: flex;
    flex: 1;
    align-items: center;
    gap: 8px;
    padding: 4px;
    background: ${color.container.highlight};
    border-radius: ${radius.m};
    transition: background-color 0.4s ease;
  }

  .addressbar-shield {
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.icon.default};
    padding: 4px;
  }

  .addressbar-url {
    flex: 1;
    font-size: 14px;
    line-height: 22px;
    letter-spacing: -0.23px;
    color: ${color.text.primary};
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    transition: color 0.4s ease;
  }

  .addressbar-menu {
    display: flex;
  }

  .menu-btn {
    display: flex;
    align-items: center;
    padding: 4px;
    border: 1px solid ${color.divider.subtle};
    border-radius: ${radius.m};
    transition: border-color 0.4s ease;
  }

  .menu-btn leo-icon {
    --leo-icon-size: 20px;
  }

  .menu-btn leo-icon:first-child {
    margin-right: -4px;
  }

  .menu-btn leo-icon:last-child {
    --leo-icon-size: 18px;
    --leo-icon-color: ${color.icon.default};
  }

  /* Browser Content */
  .browser-content {
    flex: 1;
    padding: 0 4px 4px;
    box-shadow: var(--leo-effect-elevation-01);
  }

  .browser-content-bg {
    width: 100%;
    height: 100%;
    background: ${color.container.highlight};
    border-radius: ${radius.m};
    box-shadow: 0px 1px 2px -1px rgba(0, 0, 0, 0.08), 0px 1px 3px 0px rgba(0, 0, 0, 0.08);
    transition: background-color 0.4s ease;
  }

  /* Vertical Tabs Layout Styles */
  .browser-chrome.vertical {
    display: flex;
    flex-direction: column;
  }

  .browser-chrome.vertical .browser-addressbar {
    border-top-left-radius: ${radius.m};
    border-top-right-radius: 10px;
  }

  .browser-main-content {
    display: flex;
    flex: 1;
    min-height: 0;
  }

  .vertical-sidebar {
    display: flex;
    flex-direction: column;
    width: 244px;
    background: ${color.container.background};
    overflow: hidden;
    transition: background-color 0.4s ease;
  }

  .pinned-tabs-row {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
    padding: 4px;
  }

  .pinned-tab {
    display: flex;
    align-items: center;
    justify-content: center;
    flex: 1 0 calc(50% - 2px);
    height: 32px;
    border: 1px solid ${color.divider.subtle};
    border-radius: ${radius.m};
    transition: border-color 0.4s ease;
  }

  .pinned-tab leo-icon {
    --leo-icon-size: 16px;
  }

  .sidebar-divider {
    height: 1px;
    background: ${color.divider.subtle};
    margin: 0;
    transition: background-color 0.4s ease;
  }

  .vertical-tabs-list {
    display: flex;
    flex-direction: column;
    gap: 4px;
    padding: 4px;
    flex: 1;
    overflow-x: hidden;
    overflow-y: auto;
  }

  .vertical-tab {
    display: flex;
    align-items: center;
    gap: 8px;
    height: 32px;
    padding: 0 8px;
    border-radius: ${radius.m};
  }

  .vertical-tab leo-icon {
    --leo-icon-size: 16px;
    flex-shrink: 0;
  }

  .vertical-tab.active {
    background: ${color.desktopbrowser.tabbar.activeTabVertical};
    transition: background-color 0.4s ease;
  }

  .vertical-tab-title {
    flex: 1;
    font-size: 12px;
    line-height: 18px;
    letter-spacing: -0.08px;
    color: ${color.text.primary};
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    transition: color 0.4s ease;
  }

  .vertical-tab:not(.active) .vertical-tab-title {
    color: ${color.text.secondary};
  }

  .vertical-tab-close {
    --leo-icon-size: 16px;
    flex-shrink: 0;
    color: ${color.icon.default};
    transition: color 0.4s ease;
  }

  .browser-content.vertical {
    flex: 1;
    padding: 0 4px 4px 0;
  }

  /* Preview Theme Overrides - Light Mode (Leo Desktop Browser tokens) */
  .browser-chrome.preview-theme-light {
    --preview-bg: #ffffff; /* desktopbrowser.chromeBackgroundDesktop */
    --preview-bg-highlight: #f2f2f3; /* desktopbrowser.omnibar.backgroundDesktop */
    --preview-tabbar-bg: #e4e4e5; /* desktopbrowser.tabbar.background */
    --preview-text-primary: #1c1c1d; /* text.primary */
    --preview-text-secondary: #464649; /* text.secondary */
    --preview-icon-color: #464649; /* icon.default */
    --preview-divider: #c9c9ca; /* desktopbrowser.tabbar.pinnedTabOutlineHorizontal */
    --preview-divider-vertical: #e4e4e5; /* desktopbrowser.tabbar.pinnedTabOutlineVertical */
  }

  /* Preview Theme Overrides - Dark Mode (Leo Desktop Browser tokens) */
  .browser-chrome.preview-theme-dark {
    --preview-bg: #303032; /* desktopbrowser.chromeBackgroundDesktop */
    --preview-bg-highlight: #1c1c1d; /* desktopbrowser.omnibar.backgroundDesktop */
    --preview-tabbar-bg: #1c1c1d; /* desktopbrowser.tabbar.background */
    --preview-text-primary: #e4e4e5; /* text.primary */
    --preview-text-secondary: #c9c9ca; /* text.secondary */
    --preview-icon-color: #c9c9ca; /* icon.default */
    --preview-divider: #464649; /* desktopbrowser.tabbar.pinnedTabOutlineHorizontal */
    --preview-divider-vertical: #464649; /* desktopbrowser.tabbar.pinnedTabOutlineVertical */
  }

  /* Apply theme colors to browser chrome elements */
  .browser-chrome.preview-theme-light,
  .browser-chrome.preview-theme-dark {
    background: var(--preview-bg);
  }

  .browser-chrome.preview-theme-light .browser-tabbar,
  .browser-chrome.preview-theme-dark .browser-tabbar {
    background: var(--preview-tabbar-bg);
  }

  .browser-chrome.preview-theme-light .browser-tab.active,
  .browser-chrome.preview-theme-dark .browser-tab.active {
    background: var(--preview-bg);
  }

  .browser-chrome.preview-theme-light .browser-tab.pinned,
  .browser-chrome.preview-theme-dark .browser-tab.pinned {
    border-color: var(--preview-divider);
  }

  .browser-chrome.preview-theme-light .tab-title,
  .browser-chrome.preview-theme-dark .tab-title {
    color: var(--preview-text-primary);
  }

  .browser-chrome.preview-theme-light .browser-tab:not(.active) .tab-title,
  .browser-chrome.preview-theme-dark .browser-tab:not(.active) .tab-title {
    color: var(--preview-text-secondary);
  }

  .browser-chrome.preview-theme-light .tab-close,
  .browser-chrome.preview-theme-dark .tab-close {
    color: var(--preview-icon-color);
  }

  .browser-chrome.preview-theme-light .tab-divider,
  .browser-chrome.preview-theme-dark .tab-divider {
    background: var(--preview-divider);
  }

  .browser-chrome.preview-theme-light .browser-addressbar,
  .browser-chrome.preview-theme-dark .browser-addressbar {
    background: var(--preview-bg);
  }

  .browser-chrome.preview-theme-light .toolbar-btn leo-icon,
  .browser-chrome.preview-theme-dark .toolbar-btn leo-icon {
    --leo-icon-color: var(--preview-icon-color);
  }

  .browser-chrome.preview-theme-light .addressbar-field,
  .browser-chrome.preview-theme-dark .addressbar-field {
    background: var(--preview-bg-highlight);
  }

  .browser-chrome.preview-theme-light .addressbar-url,
  .browser-chrome.preview-theme-dark .addressbar-url {
    color: var(--preview-text-primary);
  }

  .browser-chrome.preview-theme-light .menu-btn,
  .browser-chrome.preview-theme-dark .menu-btn {
    border-color: var(--preview-divider);
  }

  .browser-chrome.preview-theme-light .menu-btn leo-icon,
  .browser-chrome.preview-theme-dark .menu-btn leo-icon {
    --leo-icon-color: var(--preview-icon-color);
  }

  .browser-chrome.preview-theme-light .browser-content-bg,
  .browser-chrome.preview-theme-dark .browser-content-bg {
    background: var(--preview-bg-highlight);
  }

  .browser-chrome.preview-theme-light .browser-tab-add leo-icon,
  .browser-chrome.preview-theme-dark .browser-tab-add leo-icon {
    --leo-icon-color: var(--preview-icon-color);
  }

  .browser-chrome.preview-theme-light .browser-tab-dropdown leo-icon,
  .browser-chrome.preview-theme-dark .browser-tab-dropdown leo-icon {
    --leo-icon-color: var(--preview-icon-color);
  }

  /* Vertical layout theme overrides */
  .browser-chrome.preview-theme-light .vertical-sidebar,
  .browser-chrome.preview-theme-dark .vertical-sidebar {
    background: var(--preview-bg);
  }

  .browser-chrome.preview-theme-light .pinned-tab,
  .browser-chrome.preview-theme-dark .pinned-tab {
    border-color: var(--preview-divider-vertical);
  }

  .browser-chrome.preview-theme-light .sidebar-divider,
  .browser-chrome.preview-theme-dark .sidebar-divider {
    background: var(--preview-divider-vertical);
  }

  .browser-chrome.preview-theme-light .vertical-tab.active,
  .browser-chrome.preview-theme-dark .vertical-tab.active {
    background: var(--preview-bg-highlight);
  }

  .browser-chrome.preview-theme-light .vertical-tab-title,
  .browser-chrome.preview-theme-dark .vertical-tab-title {
    color: var(--preview-text-primary);
  }

  .browser-chrome.preview-theme-light .vertical-tab:not(.active) .vertical-tab-title,
  .browser-chrome.preview-theme-dark .vertical-tab:not(.active) .vertical-tab-title {
    color: var(--preview-text-secondary);
  }

  .browser-chrome.preview-theme-light .vertical-tab-close,
  .browser-chrome.preview-theme-dark .vertical-tab-close {
    color: var(--preview-icon-color);
  }

  .customize-options {
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};
    width: 100%;
  }

  .customize-option-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: ${spacing.m};
    padding: ${spacing.xl} ${spacing['2Xl']};
    background: ${color.material.regular};
    border-radius: ${radius.xl};
  }

  .customize-appearance-card .customize-option-row {
    background: transparent;
    border-radius: 0;
    padding: ${spacing.xl} ${spacing['2Xl']};
    border: none;
  }

  .customize-option-label {
    font: ${font.heading.h4};
    color: ${color.text.primary};
    opacity: 0.9;
  }

  .customize-appearance-card {
    display: flex;
    flex-direction: column;
    background: ${color.material.regular};
    border-radius: ${radius.xl};
  }

  .customize-divider {
    height: 1px;
    background: ${color.divider.subtle};
    margin: 0;
  }

  .customize-colors-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: ${spacing.m};
    padding: ${spacing.xl} ${spacing.xl} ${spacing.l} ${spacing.xl};
  }

  .color-swatch {
    width: 34px;
    height: 34px;
    border-radius: 50%;
    border: none;
    cursor: pointer;
    position: relative;
    overflow: hidden;
    transition: transform 0.15s ease, box-shadow 0.15s ease;
    background: conic-gradient(
      from 0deg,
      var(--swatch-color-1) 0deg 90deg,
      var(--swatch-color-2) 90deg 180deg,
      var(--swatch-color-3) 180deg 270deg,
      var(--swatch-color-1) 270deg 360deg
    );
  }

  .color-swatch:hover {
    transform: scale(1.08);
  }

  .color-swatch:focus-visible {
    outline: 2px solid ${color.primary[50]};
    outline-offset: 2px;
  }

  .color-swatch-selected {
    box-shadow: 0 0 0 1px ${color.primary[10]}, 0 0 0 3px ${color.primary[50]};
  }

  .color-swatch-check {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    --leo-icon-size: 28px;
    --leo-icon-color: ${color.primary[50]};
    background: ${color.container.background};
    border-radius:100%;
  }

  /* Segmented Control customization */
  .customize-option-row leo-segmented-control {
    --leo-segmented-control-radius: 1000px;
    --leo-segmented-control-bg: ${color.neutral[10]};
  }

  .customize-option-row leo-segmented-control-item {
    --leo-segmented-control-item-icon-gap: ${spacing.xs};
  }

  .customize-option-row leo-icon {
    --leo-icon-size: 20px;
  }
`
