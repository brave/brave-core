/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    --leo-dialog-width: 720px;
    --leo-dialog-padding: 0;
    --leo-dialog-background: ${color.container.background};

    height: 0;
  }

  h3 {
    margin: 24px 24px 16px;
  }

  .panel-body {
    display: flex;
    gap: 16px;
  }

  nav {
    flex: 0 0 244px;
    white-space: nowrap;
  }

  section {
    flex: 1 1 auto;
    padding: 10px 16px 16px;
    height: 360px;
    overflow: auto;
    overscroll-behavior: contain;
  }
`
