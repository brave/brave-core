/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    min-height: 0;
    display: flex;
    flex-direction: column;
    align-items: stretch;
    gap: 24px;
  }

  .header {
    --leo-icon-size: 40px;
    --leo-progressring-size: 24px;

    padding: ${spacing['2Xl']} ${spacing['3Xl']};
    border-radius: ${radius.xl};
    background: ${color.material.regular};
    display: flex;
    align-items: center;
    justify-content: center;
    gap: ${spacing['2Xl']};

    .carats {
      --leo-icon-size: 24px;

      display: flex;
      align-items: center;

      > :first-child {
        opacity: 0.5;
      }

      > :nth-child(2) {
        opacity: 0.75;
      }
    }

    .brave-icon {
      position: relative;
    }

    leo-progressring, .success-icon {
      position: absolute;
      inset-block-start: 24px;
      inset-inline-start: 24px;
    }

    leo-progressring {
      border-radius: ${radius.full};
      background: ${color.container.background};
      box-shadow: ${effect.elevation['01']};
    }
  }

  .data-types {
    --leo-icon-size: 24px;
    --leo-progressring-size: 24px;

    overflow-y: auto;
  }

  .data-type-name {
    flex: 1 1 auto;
  }

  .success-icon {
    --leo-icon-size: 24px;
    color: ${color.systemfeedback.successIcon};
  }
`
