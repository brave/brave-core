/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 16px;

    > * {
      display: flex;
      align-items: center;

      label {
        flex: 1 1 auto;
      }
    }
  }

  .layout-control {
    --leo-segmented-control-height: 36px;
  }

  leo-controlitem {
    --leo-control-item-padding: 4px;

    svg {
      display: block;
      height: 18px;
      width: auto;
    }
  }
`
