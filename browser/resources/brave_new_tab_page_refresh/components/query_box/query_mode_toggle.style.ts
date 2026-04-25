/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  .item {
    --leo-icon-size: 18px;
    --leo-icon-color: ${color.neutral[30]};
    --leo-segmented-control-item-padding: 4px;
    --leo-segmented-control-item-icon-gap: 4px;
    --leo-segmented-control-item-font: ${font.small.regular};
    --leo-segmented-control-item-color: ${color.neutral[30]};
  }

  .selected {
    --leo-segmented-control-item-padding: 6px;
    --leo-icon-color: ${color.neutral[50]};
    --leo-segmented-control-item-color: ${color.neutral[50]};
  }

  .icon {
    // fix issue with icons not rendering until clicked
    display: inline;
  }
`
