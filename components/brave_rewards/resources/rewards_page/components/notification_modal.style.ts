/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '../lib/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    margin-top: calc(-1 * var(--modal-header-padding-bottom) - 12px);

    @container style(--is-wide-view) {
      width: 325px;
    }
  }

  .title {
    --leo-icon-size: 40px;

    &.error {
      --leo-icon-color: ${color.systemfeedback.warningIcon};
    }

    display: flex;
    flex-direction: column;
    align-items: center;
    text-align: center;
    gap: 12px;
  }

  p {
    margin: 16px 0 24px;
    text-align: center;
  }

  .action {
    display: flex;
    flex-direction: column;
  }
`

