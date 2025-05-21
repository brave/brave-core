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
    gap: 12px;
  }

  h3 {
    --leo-icon-size: 18px;

    margin: 8px;
    display: flex;
    align-items: center;
    gap: 8px;
    font: ${font.default.semibold};
    color: #fff;
  }

  .item {
    border-radius: 8px;
    background: rgba(255, 255, 255, 0.05);
    padding: 12px 16px 16px;
  }
`
