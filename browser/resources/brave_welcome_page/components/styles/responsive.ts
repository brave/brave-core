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
      top: ${spacing['4Xl']};
      left: ${spacing['4Xl']};
    }

    .content {
      flex-direction: column;
    }

    .left-content {
      max-width: 100%;
      width: 100%;
      padding: ${spacing['4Xl']};
      padding-top: calc(${spacing['4Xl']} + 52px + ${spacing['4Xl']});
    }

    .right-content {
    min-width: auto;
      max-width: 100%;
      width: 100%;
      padding: ${spacing['4Xl']};
      overflow: scroll;
      align-items:start;
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

    .privacy-card-icon {
      display: none;
    }
  }
`

