/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    --leo-menu-max-height: max-content;

    position: absolute;
    inset-block-start: 12px;
    inset-inline-end: 12px;
  }

  leo-button {
    --leo-icon-size: 16px;
    --leo-icon-color: ${color.icon.default};

    padding: 4px;
    opacity: 0;
    transition: opacity 120ms;

    .ntp-widget:hover & {
      opacity: 1;
    }
  }

  leo-menu-item {
    --leo-icon-size: 24px;

    display: flex;
    align-items: center;
    gap: 12px;
  }
`
