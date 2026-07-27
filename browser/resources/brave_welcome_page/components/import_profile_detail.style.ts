/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'
import { wideBreakpoint } from './breakpoints'

export const style = scoped.css`
  & {
    min-height: 0;
    display: flex;
    flex-direction: column;
    align-items: stretch;
    gap: 24px;
  }

  .selected {
    --leo-icon-size: 40px;

    flex: 0 0 auto;
    padding: ${spacing['2Xl']} ${spacing['3Xl']};
    border-radius: ${radius.xl};
    background: ${color.material.regular};
    display: flex;
    align-items: center;
    gap: ${spacing['2Xl']};

    h3 {
      flex: 1 1 auto;
      opacity: 0.9;
    }

    .carat {
      --leo-icon-size: 20px;
    }
  }

  .data-types {
    leo-checkbox {
      width: 100%;
    }
  }

  @media (min-width: ${wideBreakpoint}) {
    & {
      max-height: 100%;
      overflow: hidden;
    }

    .data-types {
      flex: 1 1 auto;
      min-height: 0;
      overflow-y: auto;
    }
  }
`
