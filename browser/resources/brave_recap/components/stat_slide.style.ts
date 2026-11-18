// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  .stat {
    display: flex;
    flex-direction: column;
    align-items: center;
    padding-inline: var(--leo-spacing-2xl);
    font: var(--leo-font-heading-h1);
    text-align: center;
  }

  .big-value {
    display: flex;
    flex-wrap: wrap;
    justify-content: center;
    align-items: baseline;
  }

  .big-unit {
    white-space: pre;
  }

  .big-number {
    display: grid;
    font: var(--leo-font-heading-display1);
    font-weight: 800;
    /* Shrinks long numbers to fit; the factor is roughly a digit's width. */
    font-size: min(
      var(--recap-hero-size),
      (var(--recap-column-width) - 2 * var(--leo-spacing-2xl)) /
        (var(--chars) * 0.6)
    );
    line-height: 1;
    font-variant-numeric: tabular-nums;
    white-space: nowrap;
  }

  .big-number > * {
    grid-area: 1 / 1;
  }

  /* Invisible, but still read out: it holds the final value. */
  .big-number .sizer {
    opacity: 0;
  }
`
