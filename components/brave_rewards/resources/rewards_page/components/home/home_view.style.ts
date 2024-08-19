/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { css, scopedCSS } from '../../lib/scoped_css'

export const style = scopedCSS('home-view', css`
  & {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .columns {
    display: flex;
    gap: 24px;

    > * {
      flex: 1 1 50%;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
  }
`)
