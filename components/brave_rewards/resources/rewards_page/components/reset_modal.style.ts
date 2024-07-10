/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { css, scopedCSS } from '../lib/scoped_css'

export const style = scopedCSS('reset-modal', css`
  & {
    max-width: 536px;
    font: ${font.default.regular};
    color: ${color.text.secondary};

    a {
      font-weight: 600;
      text-decoration: none;
    }
  }

  .message-icon {
    --leo-icon-size: 40px;

    height: 88px;
    width: 88px;
    padding: 24px;
    margin-bottom: 24px;
    background: ${color.systemfeedback.warningBackground};
    border-radius: 50%;
    color: ${color.systemfeedback.warningIcon};
  }

  .text {
    padding-bottom: 16px;
  }

  .action {
    margin-top: 8px;
    align-self: center;
  }
`)

