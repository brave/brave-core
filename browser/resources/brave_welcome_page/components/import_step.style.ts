/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css``

style.passthrough.css`
  .data-types {
    padding: ${spacing['3Xl']};
    border-radius: ${radius.xl};
    background: ${color.material.regular};
    display: flex;
    flex-direction: column;
    gap: ${spacing['3Xl']};

    h4 {
      opacity: 0.9;
    }

    .list {
      border-radius: ${radius.m};
      border: solid 1px ${color.divider.subtle};
      display: flex;
      flex-direction: column;

      > * {
        min-height: calc(24px + 2 * ${spacing.xl});
        padding: ${spacing.l} ${spacing.xl};
        border-bottom: solid 1px ${color.divider.subtle};
        display: flex;
        align-items: center;
        gap: ${spacing.xl};
        line-height: 24px;

        &:last-child {
          border-bottom: none;
        }
      }
    }
  }
`
