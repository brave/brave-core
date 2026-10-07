/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { effect } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  &.toast {
    position: fixed;
    inset-block-end: 24px;
    inset-inline-start: 24px;
    z-index: 10;
    max-width: 360px;
    box-shadow: ${effect.elevation['01']};
  }
`
