/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  section {
    padding: 12px;
    display: flex;
    gap: 12px;
    align-items: center;
    flex-wrap: wrap;
    justify-content: stretch;
  }

  a {
    text-decoration: none;
  }

  .icon {
    border-radius: 8px;
    border: solid 1px ${color.divider.subtle};
    padding: 8px;

    img {
      display: block;
      height: 24px;
      width: 24px;
    }
  }

  .text {
    flex: 99 1 auto;
    display: flex;
    flex-direction: column;
  }

  .name {
    --leo-icon-size: 16px;

    display: flex;
    gap: 4px;
    align-items: center;
    font: ${font.large.semibold};
    color: ${color.text.primary};
  }

  .origin {
    --leo-icon-size: 16px;

    font: ${font.small.regular};
    color: ${color.text.tertiary};
    display: flex;
    gap: 4px;
  }
`
