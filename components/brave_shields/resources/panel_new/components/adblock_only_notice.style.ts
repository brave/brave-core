/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    margin: 0 8px;
    padding: 8px 16px 8px 24px;
    display: flex;
    align-items: center;
    gap: 16px;

    p {
      color: ${color.text.tertiary};
      font: ${font.small.regular};
    }
  }
`
