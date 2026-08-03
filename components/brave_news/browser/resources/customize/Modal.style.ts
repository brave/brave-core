/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    --leo-dialog-width: 860px;
    --leo-dialog-padding: 0;
    --leo-dialog-background: ${color.container.background};

    height: 0;
  }

  .loading {
    --leo-progressring-size: 50px;

    display: flex;
    align-items: center;
    justify-content: center;
    height: 100%;
    min-height: 400px;
  }
`
