/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`

  & {
    --leo-icon-size: 18px;

    border: solid 1px ${color.neutral[10]};
    padding: 3px;
    border-radius: ${radius.full};
    display: flex;
    align-items: center;
    gap: 4px;
  }

  button {
    color: ${color.neutral[30]};
    font: ${font.small.regular};
    border-radius: ${radius.full};
    display: flex;
    align-items: center;
    gap: 4px;
    padding: 5px 12px 5px 8px;

    .name {
      display: none;
    }

    &:hover {
      color: ${color.icon.interactive};
    }
  }

  button[disabled] {
    color: ${color.neutral[50]};
    background: ${color.neutral[10]};

    .name {
      display: block;
    }
  }

`
