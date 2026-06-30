/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    position: relative;
    display: flex;
    flex-direction: column;
    gap: 8px;
    height: 100%;
  }

  .title {
    font: ${font.components.buttonSmall};
  }

  .widget-frame {
    border: 0;
    width: 100%;
    flex: 1 1 auto;
    min-height: 180px;
    background: transparent;
    color-scheme: dark;
  }
`
