/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 8px;
    color: #fff;
  }

  h3 {
    --leo-icon-size: 18px;

    display: flex;
    align-items: center;
    gap: 4px;
    font: ${font.default.semibold};
  }

  .publishers {
    display: grid;
    grid-template-columns: repeat(3, minmax(0px, 1fr));
    gap: 24px;
  }
`
