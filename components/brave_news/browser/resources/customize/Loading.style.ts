/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    --leo-progressring-size: ${spacing['4Xl']};

    display: flex;
    align-items: center;
    justify-content: center;
    padding: ${spacing['4Xl']};
    min-height: 120px;
  }

  &.fill {
    --leo-progressring-size: 50px;

    min-height: 400px;
    height: 100%;
    padding: ${spacing['4Xl']};
  }
`
