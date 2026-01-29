/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`

  .view-toggle {
    --leo-icon-size: 20px;

    padding: 8px 16px;
    display: flex;
    align-items: center;
    gap: 16px;
    cursor: pointer;
    width: 100%;

    span {
      flex: 1 1 auto;
    }

    &:hover, [data-expanded=true] & {
      --leo-icon-color: ${color.icon.interactive};
      color: ${color.text.interactive};
    }
  }

`
