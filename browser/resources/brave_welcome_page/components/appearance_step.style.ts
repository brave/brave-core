/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  .step-ui {
    display: flex;
    flex-direction: column;
    gap: 24px;
    align-items: stretch;
  }

  h4 {
    opacity: 0.9;
  }

  .tab-layout {
    border-radius: ${radius.xl};
    padding: ${spacing.xl} ${spacing['2Xl']};
    background: ${color.material.regular};
    display: flex;
    gap: 8px;
    align-items: center;
    justify-content: space-between;
  }

  .theme {
    border-radius: ${radius.xl};
    background: ${color.material.regular};

    .mode {
      padding: ${spacing.xl} ${spacing['2Xl']};
      display: flex;
      gap: 8px;
      align-items: center;
      justify-content: space-between;
    }

    .colors {
      padding: ${spacing.l} ${spacing['2Xl']} ${spacing.l};
      border-top: solid 1px ${color.material.separator};
    }

    .theme-color {
      width: 36px;
      height: 36px;
      border-radius: ${radius.full};
    }
  }
`
