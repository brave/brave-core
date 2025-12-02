/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  .engine-icon {
    --leo-icon-size: var(--search-engine-icon-size, 16px);
    height: var(--leo-icon-size);
    width: var(--leo-icon-size);
  }

  .engine-picker-button {
    padding: 7px;
    border-radius: 4px;

    &:hover {
      background-color: ${color.container.interactive};
    }

    &.open {
      background-color: ${color.container.interactive};
      border-color: ${color.divider.interactive};
    }
  }

  leo-menu-item {
    --leo-icon-size: 20px;

    display: flex;
    align-items: center;
    gap: 16px;
    min-width: 180px;

    &[data-customize] {
      font: ${font.components.buttonSmall};
      color: ${color.text.secondary};
      justify-content: center;
    }
  }

  .divider {
    border-top: solid 1px ${color.divider.subtle};
  }
`
