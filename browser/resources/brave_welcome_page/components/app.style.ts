/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

import backgroundLight from '../assets/background_light.jpg'
import backgroundDark from '../assets/background_dark.jpg'

export const style = scoped.css`
  & {
    --app-logo-size: 52px;

    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
    background-color: ${color.primitive.neutral['0']};
    background-image: url(${backgroundLight});
    background-size: cover;
    background-position: center center;
    background-repeat: no-repeat;
    padding: ${spacing.xl};
  }

  @media (prefers-color-scheme: dark) {
    & {
      background-image: url(${backgroundDark});
    }
  }

  .container {
    position: relative;
    max-width: 1130px;
    max-height: 700px;
    width: 100%;
    height: calc(100dvh - 2 * ${spacing.xl});
    background-color: ${color.material.thick};
    border-radius: ${radius.xxl};
    backdrop-filter: blur(35px);
    overflow: hidden;
    opacity: 0;

    animation:
      container-fade-in 0.9s cubic-bezier(0.34, 1.56, 0.64, 1) 0.6s forwards;
  }

  @keyframes container-fade-in {
    0% {
      opacity: 0;
      transform: scale(0.85);
    }
    100% {
      opacity: 1;
      transform: scale(1);
    }
  }

  .logo {
    --leo-icon-size: var(--app-logo-size);
    position: absolute;
    inset-block-start: ${spacing['4Xl']};
    inset-inline-start: ${spacing['4Xl']};
    z-index: 10;
  }
`

style.passthrough.css`
  & {
    font: ${font.large.regular};
    color: ${color.text.primary};
  }

  h1 {
    font: ${font.heading.h1};
    margin: 0;
  }

  p {
    margin: 0;
  }

  .step-view {
    display: flex;
    flex-direction: column;
    height: 100%;
  }

  .step-content {
    flex: 1 1 auto;
    display: flex;
    flex-direction: column;
    padding: ${spacing['4Xl']};
    gap: ${spacing['2Xl']};
    overflow-y: auto;
  }

  .step-text {
    padding-top: calc(${spacing['4Xl']} + var(--app-logo-size));
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};
  }

  .step-ui {
    flex: 1 1 auto;
  }

  @media (min-width: 1024px) {
    .step-content {
      flex-direction: row;
      align-items: stretch;
      height: 100%;
      overflow-y: unset;
      gap: ${spacing['4Xl']};
    }

    .step-text {
      max-width: 430px;
      width: 40%;
      overflow-y: auto;
    }

    .step-ui {
      overflow-y: auto;
    }
  }
`

// View transitions rules must be global and cannot be placed in CSS scopes.
const globalStyles = new CSSStyleSheet()

globalStyles.replace(`

  @keyframes slide-forward-out {
    from {
      transform: translateX(0);
      opacity: 1;
    }
    to {
      transform: translateX(-40px);
      opacity: 0;
    }
  }

  @keyframes slide-forward-in {
    from {
      transform: translateX(40px);
      opacity: 0;
    }
    50% {
      transform: translateX(40px);
      opacity: 0;
    }
    to {
      transform: translateX(0);
      opacity: 1;
    }
  }

  @keyframes slide-back-out {
    from {
      transform: translateX(0);
      opacity: 1;
    }
    to {
      transform: translateX(40px);
      opacity: 0;
    }
  }

  @keyframes slide-back-in {
    from {
      transform: translateX(-40px);
      opacity: 0;
    }
    50% {
      transform: translateX(-40px);
      opacity: 0;
    }
    to {
      transform: translateX(0);
      opacity: 1;
    }
  }

  html:active-view-transition-type(forward, back) {
    :root {
      view-transition-name: none;
    }
    .step-content {
      view-transition-name: step-content;
    }
  }

  html:active-view-transition-type(forward) {
    &::view-transition-old(step-content) {
      animation: slide-forward-out 0.3s forwards;
    }
    &::view-transition-new(step-content) {
      animation: slide-forward-in 0.4s forwards;
    }
  }

  html:active-view-transition-type(back) {
    &::view-transition-old(step-content) {
      animation: slide-back-out 0.3s forwards;
    }
    &::view-transition-new(step-content) {
      animation: slide-back-in 0.4s forwards;
    }
  }

`)

document.adoptedStyleSheets.push(globalStyles)
