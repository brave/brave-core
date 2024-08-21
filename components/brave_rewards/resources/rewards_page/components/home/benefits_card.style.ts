/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { css, scopedCSS } from '../../lib/scoped_css'

 export const style = scopedCSS('benefits-card', css`
  h4 {
    padding: 16px;
  }

  section {
    padding: 8px;
  }

  .store {
    --leo-icon-size: 16px;
    --leo-icon-color: ${color.icon.interactive};

    display: flex;
    align-items: center;
    gap: 16px;
    text-decoration: none;
    color: ${color.text.primary};

    .icon {
      --leo-icon-size: 28px;
      --leo-icon-color: ${color.container.background};

      width: 48px;
      height: 48px;
      border-radius: 50%;
      background:
        radial-gradient(farthest-corner at center 10px,
                        rgba(255, 255, 255, 0.30) 0%,
                        rgba(255, 255, 255, 0.00) 100%),
        ${color.neutral[70]};
      display: flex;
      align-items: center;
      justify-content: center;
    }

    .text {
      flex: 1 1 auto;
    }

    .maintext {
      font: ${font.large.semibold};
      display: flex;
      gap: 8px;
      align-items: center;

      leo-label {
        position: relative;
        top: -2px;
      }
    }

    .subtext {
      font: ${font.small.regular};
      color: ${color.text.tertiary};
    }
  }
`)
