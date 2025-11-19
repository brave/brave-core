/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    background-color: ${color.systemfeedback.warningBackground};
    padding: 24px;
    border-radius: 16px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 12px;
    text-align: center;

    > * {
      max-width: 450px;
    }
  }

  .icon {
    --leo-icon-size: 24px;

    padding: 16px;
    border-radius: 50%;
    background: ${color.systemfeedback.warningVibrant};
  }

  h3 {
    margin-bottom: -4px;
  }

  p, a {
    font: ${font.small.regular};
    color: ${color.text.tertiary};
  }

  .note {
    font: ${font.xSmall.regular};
  }

  leo-button {
    width: 100%;
  }
`
