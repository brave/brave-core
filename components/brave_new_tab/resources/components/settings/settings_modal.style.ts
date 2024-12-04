/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped, global } from '../../lib/scoped_css'

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

global.css`
  @scope (${style.selector}) {

    .selected-marker {
      --leo-icon-color: #fff;
      --leo-icon-size: 24px;

      position: absolute;
      inset-block-start: 10px;
      inset-inline-end: 10px;
      background: ${color.icon.interactive};
      border-radius: 50%;
      padding: 6px;
    }

  }
`
