/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'
import { horizontalContentPadding } from '../app.style'

export const style = scoped.css`
  .new-tab-takeover-disclosure {
    color: ${color.white};
    font: ${font.xSmall.regular};
    letter-spacing: -0.08px;
    text-shadow: 0px 1px 2px ${color.elevation.secondary},
      0px 1px 3px ${color.elevation.secondary};
    white-space: nowrap;
  }

  .new-tab-takeover-disclosure-tooltip {
    font: ${font.small.regular};
    letter-spacing: -0.08px;

    max-width: min(
      359px,
      calc(
        100vw - ${2 * horizontalContentPadding}px - 2 * var(--leo-spacing-xl) -
          2px
      )
    );

    a {
      color: inherit;
    }
  }
`
