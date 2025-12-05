/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    margin: 0 8px;
    border-radius: 8px;
    padding: 16px 24px;
    background: ${color.systemfeedback.infoBackground};
    color: ${color.systemfeedback.infoText};
  }

  .actions {
    margin-top: 16px;
    display: flex;
    align-items: center;
    gap: 8px;
  }
`
