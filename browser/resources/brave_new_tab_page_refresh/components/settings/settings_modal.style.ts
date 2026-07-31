/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    --leo-dialog-width: 720px;
    --leo-dialog-padding: 0;
    --leo-dialog-background: ${color.container.background};

    height: 0;
  }

  h3 {
    padding: 24px;
    border-bottom: solid 1px ${color.divider.subtle};
  }

  .panel-body {
    display: flex;
  }

  nav {
    flex: 0 0 220px;
    white-space: nowrap;
    margin-top: 24px;
  }

  section {
    flex: 1 1 auto;
    padding: 16px;
    height: 400px;
    overflow: auto;
    overscroll-behavior: contain;
    scrollbar-width: thin;
    background: ${color.page.background};
  }
`
