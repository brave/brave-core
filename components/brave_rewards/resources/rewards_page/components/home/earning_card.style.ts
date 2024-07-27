/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { css, scopedCSS } from '../../lib/scoped_css'

export const style = scopedCSS('earning-card', css`
  .counter {
    padding: 24px 16px 16px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 20px;

    img {
      width: auto;
      height: 87px;
    }
  }

  .counter-text {
    font: ${font.default.regular};
    color: ${color.text.tertiary};
    text-align: center;
    min-height: 80px;
  }

  .counter-value {
    font: ${font.heading.h2};
    color: ${color.text.primary};
  }

  .connect {
    display: flex;
    align-items: center;
    gap: 8px;

    leo-button {
      flex: 0 0 auto;
    }
  }

  .connect-text {
    flex: 1 1 auto;
    color: ${color.text.primary};
    font: ${font.default.semibold};
  }

  .connect-subtext {
    font: ${font.small.regular};
    opacity: 0.6;
  }
`)
