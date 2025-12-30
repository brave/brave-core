/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { spacing } from '@brave/leo/tokens/css/variables'

export const responsiveStyles = `
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

