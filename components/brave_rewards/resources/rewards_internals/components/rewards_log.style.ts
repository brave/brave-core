/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  h4 {
    display: flex;
    align-items: center;
    gap: 8px;

    > * {
      flex: 0 1 auto;
    }
  }

  .title {
    flex: 1 1 auto;
  }

  textarea {
    font-family: monospace;
    font-size: 13px;
    padding: 8px;
    white-space: pre;
    height: calc(100vh - 240px);
    width: 100%;
    border-radius: 12px;
    background: ${color.container.background};
    border: none;
  }

  .auto-refresh {
    padding: 0 16px;
    font: ${font.small.semibold};
  }
`
