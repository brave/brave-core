/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    width: 100%;
    max-height: 100%;
    border-radius: ${radius.xl};
    background: ${color.material.regular};
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }

  .header {
    padding: ${spacing['2Xl']} ${spacing['3Xl']};
    display: flex;
    align-items: center;
    gap: ${spacing['2Xl']};

    .icon-grid {
      --leo-icon-size: 18px;

      display: flex;
      flex-wrap: wrap;
      width: 40px;
      gap: 4px;
    }

    h3 {
      flex: 1 1 auto;
      opacity: 0.9;
    }
  }

  .options {
    padding: 0 ${spacing.m} ${spacing.m};
    display: flex;
    flex-direction: column;
    align-items: stretch;
    gap: ${spacing.m};
    overflow-y: auto;

    > * {
      --leo-icon-size: 40px;

      padding: ${spacing.l} ${spacing['2Xl']};
      border-radius: ${radius.m};
      border: solid 1px ${color.material.separator};
      display: flex;
      gap: ${spacing['2Xl']};
      align-items: center;

      &:hover {
        background: ${color.material.thick};
        cursor: pointer;
      }

      h4 {
        flex: 1 1 auto;
        opacity: 0.9;
      }
    }
  }
`
