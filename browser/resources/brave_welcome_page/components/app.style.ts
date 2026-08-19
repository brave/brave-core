/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'
import { wideBreakpoint } from './breakpoints'

export const style = scoped.css`
  & {
    color-scheme: light dark;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
    background-image: none;
    background-size: cover;
    background-position: center center;
    background-repeat: no-repeat;
    background: linear-gradient(45deg, #200000 0%, #800000 27.45%, #FF4710 61%);
    padding: ${spacing.xl};
  }

  .welcome-container {
    position: relative;
    max-width: 1130px;
    min-width: min-content;
    max-height: 720px;
    width: 100%;
    height: calc(100dvh - 2 * ${spacing.xl});
    background-color: ${color.material.thick};
    border-radius: ${radius.xxl};
    backdrop-filter: blur(35px);
    overflow: hidden;
  }
`

style.passthrough.css`
  & {
    font: ${font.default.regular};
    color: ${color.text.primary};
  }

  h1, h2, h3, h4, p {
    margin: 0;
    text-wrap: pretty;
  }

  h1 {
    font: ${font.heading.h1};
  }

  h2 {
    font: ${font.heading.h2};
  }

  h3 {
    font: ${font.heading.h3};
  }

  h4 {
    font: ${font.heading.h4};
  }

  button {
    margin: 0;
    padding: 0;
    background: 0;
    border: none;
    text-align: unset;
    width: unset;
    font: inherit;
    cursor: pointer;

    &:disabled {
      cursor: default;
    }
  }

  .step-view {
    display: flex;
    flex-direction: column;
    height: 100%;
  }

  .step-content {
    flex: 1 1 auto;
    min-height: 0;
    display: flex;
    flex-direction: column;
    padding: ${spacing['4Xl']};
    gap: ${spacing['2Xl']};
    overflow-y: auto;
  }

  .step-text {
    font: ${font.large.regular};
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};

    a {
      color: inherit;
      text-decoration: underline;
    }
  }

  .step-header {
    --leo-icon-size: 52px;

    padding-bottom: calc(${spacing['4Xl']} - ${spacing['2Xl']});
  }

  .step-ui {
    flex: 1 1 auto;
  }

  .step-view footer {
    width: 100%;
    padding: ${spacing['2Xl']};
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: ${spacing['m']};
    background:
      linear-gradient(to bottom, ${color.material.thick}, transparent);

    .forward {
      margin-left: auto;
      display: flex;
      gap: ${spacing['m']};
      align-items: center;
      text-align: end;

      leo-button[kind=filled]:last-child {
        min-width: 240px;
      }
    }
  }

  @media (min-width: ${wideBreakpoint}) {
    .step-content {
      flex-direction: row;
      gap: ${spacing['4Xl']};
    }

    .step-text {
      max-width: 350px;
      align-self: stretch;
    }

    .step-ui {
      align-self: safe center;
    }
  }
`
