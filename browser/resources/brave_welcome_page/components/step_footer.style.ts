/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    width: 100%;
    padding: ${spacing['2Xl']};
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: ${spacing['m']};
    background:
      linear-gradient(to bottom, ${color.material.thick}, transparent);
  }

  .forward {
    display: flex;
    gap: ${spacing['m']};
    align-items: center;
    text-align: end;
  }

  .main-button {
    min-width: 240px;
  }
`
