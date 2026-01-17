/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  .actions {
    padding: 16px;
    display: flex;
    align-items: center;
    gap: 8px;
    border-top: solid 1px ${color.divider.subtle};
  }

  .try-off {
    padding: 12px 8px 16px;
    border-top: solid 1px ${color.divider.subtle};
    background: ${color.page.background};
    color: ${color.text.tertiary};
    font: ${font.small.regular};
    text-align: center;

    button {
      text-decoration: underline;
    }
  }
`
