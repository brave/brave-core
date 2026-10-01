/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  .new-tab-takeover-disclosure {
    color: ${color.white};
    font: ${font.small.regular};
    text-shadow: 0px 1px 4px rgba(0, 0, 0, 0.40);
    white-space: nowrap;
  }

  .new-tab-takeover-disclosure-tooltip {
    max-width: 320px;

    a {
      color: inherit;
    }
  }

  /* Collapses the tooltip's shadow-DOM bubble, which stays visible even
     when the text inside it is hidden. */
  :scope:has(:popover-open) {
    --leo-tooltip-background: transparent;
    --leo-tooltip-padding: 0;
    --leo-tooltip-shadow: none;
  }

  :scope:has(:popover-open) .new-tab-takeover-disclosure-tooltip {
    display: none;
  }
`
