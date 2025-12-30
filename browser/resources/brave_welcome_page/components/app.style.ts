/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

import bgLight from './img/bg-light.jpg'
import bgDark from './img/bg-dark.jpg'

export const style = scoped.css`
  /* CSS Reset */
  *,
  *::before,
  *::after {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
  }

  img,
  picture,
  video,
  canvas,
  svg {
    display: block;
    max-width: 100%;
  }

  input,
  button,
  textarea,
  select {
    font: inherit;
  }

  p,
  h1,
  h2,
  h3,
  h4,
  h5,
  h6 {
    overflow-wrap: break-word;
  }

  a {
    color: inherit;
    text-decoration: underline;
  }

  ul,
  ol {
    list-style: none;
  }

  & {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
    font: ${font.large.regular};
    color: ${color.text.primary};
    background-color: ${color.primitive.neutral['0']};
    background-image: url(${bgLight});
    background-size: cover;
    background-position: center center;
    background-repeat: no-repeat;
    padding: ${spacing.xl};
  }

  @media (prefers-color-scheme: dark) {
    & {
      background-image: url(${bgDark});
    }
  }

  h1 {
    font: ${font.heading.h1};
    margin: 0;
    margin-bottom: ${spacing['2Xl']};
  }
  p{
  margin-bottom: ${spacing['l']};
  }  

  .container {
    display: flex;
    flex-direction: column;
    flex-wrap: nowrap;
    max-width: 1130px;
    max-height: 700px;
    width: 100%;
    height: calc(100dvh - 2 * ${spacing.xl});
    background-color: ${color.material.thick};
    border-radius: ${radius.xxl};
    backdrop-filter: blur(35px);
    overflow: hidden;
  }

  /* Initial page entrance animation */
  .container.entrance-animation {
    opacity: 0;
    animation: entranceFadeScale 0.9s cubic-bezier(0.34, 1.56, 0.64, 1) 0.6s forwards;
  }

  @keyframes entranceFadeScale {
    0% {
      opacity: 0;
      transform: scale(0.85);
    }
    100% {
      opacity: 1;
      transform: scale(1);
    }
  }

  .content-area {
    display: flex;
    flex: 1;
    position: relative;
    overflow: hidden;
  }

  /* Static Brave logo - stays in place during transitions */
  .content-area > .brave-logo-container {
    position: absolute;
    top: ${spacing['4Xl']};
    left: ${spacing['4Xl']};
    z-index: 10;
  }

  /* Step transition wrapper */
  .step-wrapper {
    display: flex;
    flex-direction: column;
    flex: 1;
    transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1),
                opacity 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  }

  .step-visible {
    transform: translateX(0);
    opacity: 1;
  }

  /* Exiting animations */
  .step-exit-left {
    transform: translateX(-40px);
    opacity: 0;
  }

  .step-exit-right {
    transform: translateX(40px);
    opacity: 0;
  }

  /* Entering animations */
  .step-enter-from-right {
    animation: slideInFromRight 0.3s cubic-bezier(0.4, 0, 0.2, 1) forwards;
  }

  .step-enter-from-left {
    animation: slideInFromLeft 0.3s cubic-bezier(0.4, 0, 0.2, 1) forwards;
  }

  @keyframes slideInFromRight {
    from {
      transform: translateX(40px);
      opacity: 0;
    }
    to {
      transform: translateX(0);
      opacity: 1;
    }
  }

  @keyframes slideInFromLeft {
    from {
      transform: translateX(-40px);
      opacity: 0;
    }
    to {
      transform: translateX(0);
      opacity: 1;
    }
  }

  .content {
    display: flex;
    width: 100%;
    flex: 1;
  }

  .left-content {
    max-width: 430px;
    width: 40%;
    padding: ${spacing['4Xl']};
    /* Add top padding to account for the absolutely positioned logo */
    padding-top: calc(${spacing['4Xl']} + 52px + ${spacing['4Xl']});
  }

  .brave-logo-container {
    margin-bottom: ${spacing['4Xl']};
  }

  .brave-logo-container leo-icon {
    --leo-icon-size: 52px;
  }

  .left-text-content {
    display: flex;
    flex-direction: column;
  }

  .right-content {
    max-width: 700px;
    width: 60%;
    padding: ${spacing['4Xl']};
    display: flex;
    align-items: center;
    justify-content: center;
  }

  /* Welcome page hero image */
  .right-content.welcome-hero {
    padding: 0;
    overflow: hidden;
    flex: 1;
    min-height: 0;
  }

  .hero-image {
    width: 100%;
    height: 100%;
    object-fit: cover;
    display: block;
  }

  /* Browser Dropdown Styles */
  .browser-dropdown {
    display: flex;
    flex-direction: column;
    background: ${color.material.regular};
    border-radius: ${radius.xl};
    overflow: hidden;
    width: 100%;
    animation: fadeIn 0.25s ease-out;
  }

  @keyframes fadeIn {
    from {
      opacity: 0;
      transform: translateY(-8px);
    }
    to {
      opacity: 1;
      transform: translateY(0);
    }
  }

  .browser-dropdown-header {
    display: flex;
    align-items: center;
    gap: ${spacing['2Xl']};
    padding: ${spacing['2Xl']} ${spacing['3Xl']};
  }

  .browser-icons-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 4px;
    width: 40px;
    height: 40px;
    flex-shrink: 0;
  }

  .browser-icons-grid leo-icon {
    --leo-icon-size: 18px;
    --leo-icon-color: ${color.icon.default};
  }

  .browser-dropdown-header h3 {
    font: ${font.heading.h3};
    color: ${color.text.primary};
    opacity: 0.9;
    margin: 0;
  }

  .browser-dropdown-list {
    display: flex;
    flex-direction: column;
    gap: ${spacing.m};
    padding: 0 ${spacing.m} ${spacing.m};
    max-height: 432px;
    overflow-y: auto;
  }

  .browser-item {
    display: flex;
    align-items: center;
    gap: ${spacing['2Xl']};
    padding: ${spacing.l} ${spacing['2Xl']};
    border: 1px solid ${color.divider.subtle};
    border-radius: ${radius.m};
    cursor: pointer;
    transition: background-color 0.15s ease;
  }

  .browser-item:hover {
    background: ${color.material.thick};
  }

  .browser-item-icon {
    width: 40px;
    height: 40px;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-shrink: 0;
  }

  .browser-item-icon leo-icon {
    --leo-icon-size: 40px;
    --leo-icon-color: ${color.icon.default};
  }

  .browser-item-name {
    flex: 1;
    font: ${font.heading.h4};
    color: ${color.text.primary};
    opacity: 0.9;
  }

  /* Import container - wraps both states */
  .import-container {
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};
    width: 100%;
  }

  /* Selected browser header */
  .browser-selected-header {
    display: flex;
    align-items: center;
    gap: ${spacing['2Xl']};
    padding: ${spacing['2Xl']} ${spacing['3Xl']};
    background: ${color.material.regular};
    border-radius: ${radius.xl};
    cursor: pointer;
    animation: fadeIn 0.25s ease-out;
    transition: opacity 0.2s ease-out, transform 0.2s ease-out;
  }

  .browser-selected-header.exiting {
    opacity: 0;
    transform: scale(0.98);
  }

  .browser-selected-header h3 {
    flex: 1;
    font: ${font.heading.h3};
    color: ${color.text.primary};
    opacity: 0.9;
    margin: 0;
  }

  .browser-selected-header .browser-item-icon {
    width: 40px;
    height: 40px;
  }

  .browser-selected-header .browser-item-icon leo-icon {
    --leo-icon-size: 40px;
  }

  .browser-selected-header > leo-icon:last-child {
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.icon.default};
  }

  /* Import options container */
  .import-options {
    display: flex;
    flex-direction: column;
    gap: ${spacing['3Xl']};
    padding: ${spacing['3Xl']};
    background: ${color.material.regular};
    border-radius: ${radius.xl};
    animation: fadeIn 0.25s ease-out 0.1s both;
  }

  .import-options h4 {
    font: ${font.heading.h4};
    color: ${color.text.primary};
    opacity: 0.9;
    margin: 0;
  }

  .import-options-list {
    display: flex;
    flex-direction: column;
    border: 1px solid ${color.divider.subtle};
    border-radius: ${radius.m};
    overflow: hidden;
  }

  .import-option-item {
    display: flex;
    align-items: center;
    padding: ${spacing.l} ${spacing.xl};
    border-bottom: 1px solid ${color.divider.subtle};
    cursor: pointer;
  }

  .import-option-item:last-child {
    border-bottom: none;
  }

  /* Import progress state styles */
  .import-option-item.importing-state {
    cursor: default;
    gap: ${spacing.xl};
  }

  .import-status-icon {
    width: 24px;
    height: 24px;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-shrink: 0;
  }

  .import-status-icon leo-progressring {
    --leo-progressring-size: 24px;
  }

  .import-status-icon leo-icon {
    --leo-icon-size: 24px;
  }

  .import-status-icon .status-complete {
    --leo-icon-color: ${color.systemfeedback.successIcon};
  }

  .import-item-label {
    flex: 1;
    font: ${font.default.regular};
    color: ${color.text.primary};
    opacity: 0.9;
  }

  .import-item-status {
    font: ${font.default.regular};
    color: ${color.text.tertiary};
    text-align: right;
  }

  /* Browser transfer header - shown during import */
  .browser-transfer-header {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: ${spacing['2Xl']};
    padding: ${spacing['2Xl']} ${spacing['3Xl']};
    background: ${color.material.regular};
    border-radius: ${radius.xl};
    animation: fadeIn 0.25s ease-out;
  }

  .browser-transfer-header .browser-item-icon {
    width: 40px;
    height: 40px;
    animation: browserIconSlideIn 0.5s cubic-bezier(0.34, 1.56, 0.64, 1) forwards;
  }

  @keyframes browserIconSlideIn {
    from {
      transform: translateX(-80px);
    }
    to {
      transform: translateX(0);
    }
  }

  .browser-transfer-header .browser-item-icon leo-icon {
    --leo-icon-size: 40px;
  }

  .transfer-arrows {
    display: flex;
    align-items: center;
    animation: transferElementsFadeIn 0.4s ease-out 0.15s both;
  }

  @keyframes transferElementsFadeIn {
    from {
      opacity: 0;
      transform: translateX(-10px);
    }
    to {
      opacity: 1;
      transform: translateX(0);
    }
  }

  .transfer-arrows leo-icon {
    --leo-icon-size: 24px;
    --leo-icon-color: ${color.icon.default};
  }

  .transfer-arrows .arrow-1 {
    opacity: 0.5;
    animation: arrowPulse 1.5s ease-in-out infinite 0.6s;
  }

  .transfer-arrows .arrow-2 {
    opacity: 0.75;
    animation: arrowPulse 1.5s ease-in-out infinite 0.8s;
  }

  .transfer-arrows .arrow-3 {
    opacity: 1;
    animation: arrowPulse 1.5s ease-in-out infinite 1s;
  }

  @keyframes arrowPulse {
    0%, 100% {
      opacity: 0.3;
    }
    50% {
      opacity: 1;
    }
  }

  .brave-icon-with-spinner {
    position: relative;
    width: 40px;
    height: 40px;
    animation: braveIconFadeIn 0.4s ease-out 0.25s both;
  }

  @keyframes braveIconFadeIn {
    from {
      opacity: 0;
      transform: scale(0.8);
    }
    to {
      opacity: 1;
      transform: scale(1);
    }
  }

  .brave-icon-with-spinner > leo-icon {
    --leo-icon-size: 40px;
  }

  .transfer-spinner {
    position: absolute;
    bottom: -4px;
    right: -4px;
    background: ${color.container.background};
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    box-shadow: 0px 1px 3px rgba(0, 0, 0, 0.08), 0px 1px 2px -1px rgba(0, 0, 0, 0.08);
    animation: spinnerFadeIn 0.3s ease-out 0.5s both;
  }

  @keyframes spinnerFadeIn {
    from {
      opacity: 0;
      transform: scale(0.5);
    }
    to {
      opacity: 1;
      transform: scale(1);
    }
  }

  .transfer-spinner leo-progressring {
    --leo-progressring-size: 24px;
  }

  .transfer-complete {
    position: absolute;
    bottom: -4px;
    right: -4px;
    background: ${color.container.background};
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    box-shadow: 0px 1px 3px rgba(0, 0, 0, 0.08), 0px 1px 2px -1px rgba(0, 0, 0, 0.08);
    animation: spinnerFadeIn 0.3s ease-out both;
  }

  .transfer-complete leo-icon {
    --leo-icon-size: 24px;
    --leo-icon-color: ${color.systemfeedback.successIcon};
  }

  /* Privacy Cards Styles */
  .privacy-cards {
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};
    width: 100%;
  }

  .privacy-card {
    display: flex;
    align-items: center;
    gap: ${spacing['2Xl']};
    padding: ${spacing['2Xl']};
    background: ${color.material.regular};
    border-radius: ${radius.xl};
    cursor: pointer;
    transition: background-color 0.15s ease, filter 0.3s ease;
  }

  .privacy-card:hover {
    background: ${color.material.thick};
  }

  .privacy-card-icon {
    width: 120px;
    height: 100px;
    border-radius: ${radius.l};
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .privacy-card-content {
    display: flex;
    flex-direction: column;
    gap: ${spacing.s};
    flex: 1;
  }

  .privacy-card-title {
    font: ${font.heading.h3};
    color: ${color.text.primary};
    opacity: 0.9;
    margin: 0;
    transition: color 0.3s ease;
  }

  .privacy-card-subtitle {
    font: ${font.default.regular};
    color: ${color.text.secondary};
    margin: 0;
    transition: color 0.3s ease;
  }

  .privacy-card leo-checkbox {
    flex-shrink: 0;
  }

  .privacy-card-unchecked {
    filter: grayscale(1);
  }
  .privacy-card-unchecked .privacy-card-icon {
  opacity: 0.5;
}

  .privacy-card-unchecked .privacy-card-title,
  .privacy-card-unchecked .privacy-card-subtitle {
    color: ${color.text.tertiary};
  }

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
    width: 100%;
    height: 100%;
    object-fit: cover;
    filter: blur(6px);
    transform: scale(1.05);
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
  }

  .browser-tab:not(.active) .tab-title {
    color: ${color.text.secondary};
  }

  .tab-close {
    --leo-icon-size: 16px;
    flex-shrink: 0;
    color: ${color.icon.default};
  }

  .tab-divider {
    position: absolute;
    right: 0;
    top: 50%;
    transform: translateY(-50%);
    width: 1px;
    height: 24px;
    background: ${color.divider.subtle};
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
  }

  .browser-content-bg {
    width: 100%;
    height: 100%;
    background: ${color.container.highlight};
    border-radius: ${radius.m};
    box-shadow: 0px 1px 2px -1px rgba(0, 0, 0, 0.08), 0px 1px 3px 0px rgba(0, 0, 0, 0.08);
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
  }

  .pinned-tab leo-icon {
    --leo-icon-size: 16px;
  }

  .sidebar-divider {
    height: 1px;
    background: ${color.divider.subtle};
    margin: 0;
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
    background: ${color.container.highlight};
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
  }

  .vertical-tab:not(.active) .vertical-tab-title {
    color: ${color.text.secondary};
  }

  .vertical-tab-close {
    --leo-icon-size: 16px;
    flex-shrink: 0;
    color: ${color.icon.default};
  }

  .browser-content.vertical {
    flex: 1;
    padding: 0 4px 4px 0;
  }

  /* Preview Theme Overrides - Light Mode */
  .browser-chrome.preview-theme-light {
    --preview-bg: #FFFFFF;
    --preview-bg-highlight: #F0F2F5;
    --preview-tabbar-bg: #E3E5E7;
    --preview-text-primary: #1D1F25;
    --preview-text-secondary: #6B6F7B;
    --preview-icon-color: #6B6F7B;
    --preview-divider: rgba(0, 0, 0, 0.08);
  }

  /* Preview Theme Overrides - Dark Mode */
  .browser-chrome.preview-theme-dark {
    --preview-bg: #1E2029;
    --preview-bg-highlight: #2B2E3A;
    --preview-tabbar-bg: #14151A;
    --preview-text-primary: #F0F2F5;
    --preview-text-secondary: #9A9DAA;
    --preview-icon-color: #9A9DAA;
    --preview-divider: rgba(255, 255, 255, 0.1);
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

  .browser-chrome.preview-theme-light .menu-btn leo-icon:last-child,
  .browser-chrome.preview-theme-dark .menu-btn leo-icon:last-child {
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
    border-color: var(--preview-divider);
  }

  .browser-chrome.preview-theme-light .sidebar-divider,
  .browser-chrome.preview-theme-dark .sidebar-divider {
    background: var(--preview-divider);
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
    padding: ${spacing.l} ${spacing.xl} ${spacing.xl};
  }

  .color-swatch {
    width: 36px;
    height: 36px;
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
    --leo-segmentedcontrol-radius: 1000px;
    --leo-segmentedcontrol-background: ${color.neutral[10]};
  }

  .customize-option-row leo-segmented-control-item {
    --leo-segmentedcontrolitem-icon-gap: ${spacing.xs};
  }

  .customize-option-row leo-icon {
    --leo-icon-size: 20px;
  }

  .footer {
    display: flex;
    width: 100%;
    padding: ${spacing['2Xl']};
    align-items: center;
    flex-shrink: 0;
    background: linear-gradient(to bottom, ${color.material.thick}, transparent);
    justify-content: space-between;
  }

  .footer-left {
    display: flex;
    gap: ${spacing['2Xl']};
  }

  .footer-right {
    display: flex;
    gap: ${spacing['m']};
    text-align: right;
  }

  .main-button {
    min-width: 240px;
  }

  /* Tablet breakpoints */
  @media (max-width: 1024px) {
    .container {
      max-height: calc(100dvh - 2 * ${spacing.xl});
      height: auto;
    }

    .content-area > .brave-logo-container {
      top: ${spacing['2Xl']};
      left: ${spacing['2Xl']};
    }

    .content {
      flex-direction: column;
    }

    .left-content {
      max-width: 100%;
      width: 100%;
      padding: ${spacing['2Xl']};
      padding-top: calc(${spacing['2Xl']} + 52px + ${spacing['2Xl']});
    }

    .right-content {
      max-width: 100%;
      width: 100%;
      padding: ${spacing['2Xl']};
      overflow: hidden;
    }

    .right-content.welcome-hero {
      padding: 0;
    }

    .mock-window-preview {
      max-height: 255px;
      flex: 0 0 auto;
      height: 255px;
    }

    .customize-content {
      max-width: 100%;
      height: auto;
    }

    .customize-options {
      width: 100%;
    }

    .customize-option-row {
      flex-wrap: wrap;
      gap: ${spacing.l};
    }

    .customize-option-label {
      flex: 1 0 100%;
    }

    .customize-option-row leo-segmented-control {
      width: 100%;
    }

    .customize-colors-row {
      flex-wrap: wrap;
      justify-content: flex-start;
      gap: ${spacing.l};
    }

    .color-swatch {
      flex: 0 0 auto;
    }

    .footer {
      flex-direction: column;
      gap: ${spacing['m']};
      padding: ${spacing['xl']};
    }

    .footer-left,
    .footer-right {
      flex-direction: column;
      width: 100%;
    }

    .footer-right {
      order: -1;
    }

    .main-button {
      order: -1;
    }

    .privacy-card-icon {
      display: none;
    }
  }
`
