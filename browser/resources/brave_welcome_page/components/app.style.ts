/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

import bgLight from './img/bg-light.jpg'

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
