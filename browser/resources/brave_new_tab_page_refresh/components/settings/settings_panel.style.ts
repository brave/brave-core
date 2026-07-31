/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    background: ${color.container.background};
    box-shadow: ${effect.elevation['01']};
    border-radius: 8px;
  }
`

style.passthrough.css`
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

  .control-row,
  .toggle-row {
    padding: 24px;
    border-bottom: solid 1px ${color.divider.subtle};

    &:last-child {
      border-bottom: none;
    }
  }

  .control-row {
    display: flex;
    align-items: center;
    gap: 8px;

    label {
      flex: 1 1 auto;
    }
  }

  .toggle-row {
    --leo-toggle-label-flex-direction: row-reverse;

    .label {
      flex: 1 1 auto;
    }
  }
`
