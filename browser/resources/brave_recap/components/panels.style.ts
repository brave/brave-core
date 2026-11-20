// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { scoped } from '$web-common/scoped_css'
import { LANDSCAPE_QUERY } from './app.style'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: var(--leo-spacing-s);
    padding-inline: var(--leo-spacing-2xl);
  }

  .panel {
    padding: var(--leo-spacing-xl);
    border-radius: var(--leo-radius-xxl);
    background: var(--recap-glass);
    backdrop-filter: var(--recap-glass-blur);
    text-align: center;
    /* Fading the panel itself, not a parent, keeps its blur. */
    --recap-panel-from: var(--leo-spacing-l);
    animation: recap-panel-in var(--leo-duration-l) var(--leo-easing-out)
      var(--leo-duration-s) both;
  }

  .panel.comparison {
    --recap-panel-from: calc(-1 * var(--leo-spacing-l));
    padding-block: var(--leo-spacing-2xl);
    font: var(--leo-font-heading-h1);
    text-wrap: balance;
    border-end-start-radius: var(--leo-radius-m);
    border-end-end-radius: var(--leo-radius-m);
  }

  .panel.note {
    font: var(--leo-font-heading-h4);
  }

  .panel.comparison + .panel.note {
    border-start-start-radius: var(--leo-radius-m);
    border-start-end-radius: var(--leo-radius-m);
  }

  /* After navigation, panels wait for the slide to land, so the two motions
   * don't add up. */
  @media (prefers-reduced-motion: no-preference) {
    .slide-frame[data-motion='enter'] & .panel {
      animation-delay: var(--leo-duration-l);
    }

    .panel.comparison + .panel.note {
      animation-delay: calc(2 * var(--leo-duration-s));
    }

    .slide-frame[data-motion='enter'] & .panel.comparison + .panel.note {
      animation-delay: calc(var(--leo-duration-l) + var(--leo-duration-s));
    }
  }

  @media ${LANDSCAPE_QUERY} {
    & {
      padding-inline: 0 var(--leo-spacing-2xl);
    }

    .panel {
      padding-inline: var(--leo-spacing-2xl);
      text-align: start;
    }
  }

  @media (prefers-reduced-motion: reduce) {
    .panel {
      animation: fade-in var(--leo-duration-s) var(--leo-easing-out);
    }
  }
`
