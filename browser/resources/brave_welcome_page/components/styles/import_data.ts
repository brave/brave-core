/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'

export const importDataStyles = `
  /* Browser Selector Wrapper - contains both states */
  .browser-selector-wrapper {
    position: relative;
    width: 100%;
  }

  /* Browser Dropdown Styles */
  .browser-dropdown {
    display: flex;
    flex-direction: column;
    background: ${color.material.regular};
    border-radius: ${radius.xl};
    overflow: hidden;
    width: 100%;
    position: relative;
  }

  .browser-dropdown::after {
    content: '';
    position: absolute;
    bottom: 0;
    left: 0;
    right: 0;
    height: 48px;
    background: linear-gradient(to top, ${color.material.thick}, transparent);
    pointer-events: none;
    border-radius: 0 0 ${radius.xl} ${radius.xl};
    opacity: 1;
    transition: opacity 0.2s ease;
  }

  .browser-dropdown.scrolled-to-bottom::after {
    opacity: 0;
  }

  .browser-dropdown.entering {
    animation: dropdownEnter 0.3s cubic-bezier(0.34, 1.56, 0.64, 1) forwards;
  }

  .browser-dropdown.exiting {
    animation: dropdownExit 0.2s cubic-bezier(0.4, 0, 0.2, 1) forwards;
  }

  @keyframes dropdownEnter {
    from {
      opacity: 0;
      transform: translateY(-12px) scale(0.96);
    }
    to {
      opacity: 1;
      transform: translateY(0) scale(1);
    }
  }

  @keyframes dropdownExit {
    from {
      opacity: 1;
      transform: translateY(0) scale(1);
    }
    to {
      opacity: 0;
      transform: translateY(8px) scale(0.98);
    }
  }

  /* Browser item staggered entrance animation */
  .browser-dropdown.entering .browser-item {
    opacity: 0;
    animation: browserItemEnter 0.25s cubic-bezier(0.34, 1.56, 0.64, 1) forwards;
  }

  @keyframes browserItemEnter {
    from {
      opacity: 0;
      transform: translateX(-8px);
    }
    to {
      opacity: 1;
      transform: translateX(0);
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

  /* Morphing header - handles both selected and transfer states */
  .browser-morphing-header {
    display: flex;
    align-items: center;
    justify-content: center;
    padding: ${spacing['2Xl']} ${spacing['3Xl']};
    background: ${color.material.regular};
    border-radius: ${radius.xl};
    position: relative;
    min-height: 88px;
    overflow: hidden;
  }

  .browser-morphing-header.selected-mode {
    cursor: pointer;
  }

  /* Browser icon - shared element that animates position */
  .browser-morphing-header .browser-item-icon {
    width: 40px;
    height: 40px;
    flex-shrink: 0;
    z-index: 2;
    position: absolute;
    transition: left 0.6s cubic-bezier(0.34, 1.2, 0.64, 1);
  }

  .browser-morphing-header.selected-mode .browser-item-icon {
    left: ${spacing['3Xl']};
  }

  .browser-morphing-header.transfer-mode .browser-item-icon {
    left: calc(50% - 100px - ${spacing['2Xl']});
  }

  .browser-morphing-header .browser-item-icon leo-icon {
    --leo-icon-size: 40px;
  }

  /* Selected state elements container */
  .browser-morphing-header .selected-elements {
    display: flex;
    align-items: center;
    gap: ${spacing['2Xl']};
    position: absolute;
    left: calc(${spacing['3Xl']} + 40px + ${spacing['2Xl']});
    right: ${spacing['3Xl']};
    transition: opacity 0.25s ease, transform 0.25s ease;
  }

  .browser-morphing-header.selected-mode .selected-elements {
    opacity: 1;
    transform: translateX(0);
    pointer-events: auto;
  }

  .browser-morphing-header.transfer-mode .selected-elements {
    opacity: 0;
    transform: translateX(-20px);
    pointer-events: none;
  }

  .browser-morphing-header .selected-elements h3 {
    flex: 1;
    font: ${font.heading.h3};
    color: ${color.text.primary};
    opacity: 0.9;
    margin: 0;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .browser-morphing-header .selected-elements > leo-icon:last-child {
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.icon.default};
    transition: transform 0.2s ease;
    flex-shrink: 0;
  }

  .browser-morphing-header.selected-mode:hover .selected-elements > leo-icon:last-child {
    transform: translateY(2px);
  }

  /* Transfer state elements container */
  .browser-morphing-header .transfer-elements {
    display: flex;
    align-items: center;
    gap: ${spacing.xl};
    transition: opacity 0.35s ease 0.15s, transform 0.35s ease 0.15s;
  }

  .browser-morphing-header.selected-mode .transfer-elements {
    opacity: 0;
    transform: translateX(30px);
    pointer-events: none;
  }

  .browser-morphing-header.transfer-mode .transfer-elements {
    opacity: 1;
    transform: translateX(0);
    pointer-events: auto;
  }

  /* Entering animations for morphing header */
  .browser-morphing-header.entering {
    animation: morphingHeaderEnter 0.35s cubic-bezier(0.34, 1.56, 0.64, 1) forwards;
  }

  .browser-morphing-header.entering .browser-item-icon {
    animation: selectedIconBounce 0.4s cubic-bezier(0.34, 1.56, 0.64, 1) 0.1s both;
  }

  @keyframes morphingHeaderEnter {
    from {
      opacity: 0;
      transform: translateY(12px) scale(0.96);
    }
    to {
      opacity: 1;
      transform: translateY(0) scale(1);
    }
  }

  @keyframes selectedIconBounce {
    from {
      transform: scale(0.7);
      opacity: 0;
    }
    50% {
      transform: scale(1.1);
    }
    to {
      transform: scale(1);
      opacity: 1;
    }
  }

  /* Exiting animations */
  .browser-morphing-header.exiting {
    animation: morphingHeaderExit 0.2s cubic-bezier(0.4, 0, 0.2, 1) forwards;
  }

  @keyframes morphingHeaderExit {
    from {
      opacity: 1;
      transform: translateY(0) scale(1);
    }
    to {
      opacity: 0;
      transform: translateY(-8px) scale(0.98);
    }
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

  /* Transfer elements styles (inside morphing header) */
  .transfer-arrows {
    display: flex;
    align-items: center;
  }

  .transfer-arrows leo-icon {
    --leo-icon-size: 24px;
    --leo-icon-color: ${color.icon.default};
  }

  .browser-morphing-header.transfer-mode .transfer-arrows .arrow-1 {
    animation: arrowPulse 1.5s ease-in-out infinite 0.6s;
  }

  .browser-morphing-header.transfer-mode .transfer-arrows .arrow-2 {
    animation: arrowPulse 1.5s ease-in-out infinite 0.8s;
  }

  .browser-morphing-header.transfer-mode .transfer-arrows .arrow-3 {
    animation: arrowPulse 1.5s ease-in-out infinite 1s;
  }

  /* Stop animation when import is complete */
  .browser-morphing-header.import-complete .transfer-arrows .arrow-1,
  .browser-morphing-header.import-complete .transfer-arrows .arrow-2,
  .browser-morphing-header.import-complete .transfer-arrows .arrow-3 {
    animation: none;
    opacity: 1;
  }

  /* Apply grayscale to browser icon when import is complete */
  .browser-morphing-header.import-complete .browser-item-icon {
    filter: grayscale(1);
    opacity: 0.5;
    transition: all 0.3s ease;
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
  }

  .brave-icon-with-spinner > leo-icon {
    --leo-icon-size: 40px;
  }

  .transfer-spinner {
    position: absolute;
    bottom: -4px;
    right: -12px;
    background: ${color.container.background};
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    box-shadow: 0px 1px 3px rgba(0, 0, 0, 0.08), 0px 1px 2px -1px rgba(0, 0, 0, 0.08);
    animation: spinnerFadeIn 0.3s ease-out both;
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
    right: -12px;
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
`

