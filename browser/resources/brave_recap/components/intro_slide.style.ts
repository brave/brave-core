// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { scoped } from '$web-common/scoped_css'
import { LANDSCAPE_QUERY } from './app.style'

export const style = scoped.css`
  .intro {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: var(--leo-spacing-3xl);
    padding: var(--leo-spacing-4xl);
    text-align: center;
  }

  .intro h2 {
    font: var(--leo-font-heading-display3);
    text-wrap: balance;
  }

  .app-icon {
    --app-icon-size: 88px;
    display: grid;
    place-items: center;
    width: var(--app-icon-size);
    height: var(--app-icon-size);
    border-radius: var(--leo-radius-xxl);
    background: var(--leo-color-white);
    /* The release logo's artwork fills most, but not all, of its square. */
    --leo-icon-size: calc(var(--app-icon-size) * 0.93);
  }

  .messages {
    display: flex;
    flex-direction: column;
    gap: var(--leo-spacing-xl);
  }

  .message {
    padding-block: var(--leo-spacing-2xl);
    font: var(--leo-font-heading-h2);
    background: var(--recap-glass);
    backdrop-filter: var(--recap-glass-blur);
    animation: recap-message-in var(--leo-duration-l) var(--leo-easing-out) both;
  }

  @keyframes recap-message-in {
    from {
      transform: translateX(var(--recap-message-from));
    }
  }

  .message strong {
    font-weight: 700;
    color: var(--recap-highlight);
  }

  .message.start {
    --recap-message-from: calc(-100% * var(--recap-inline));
    margin-inline-end: var(--leo-spacing-2xl);
    padding-inline: var(--leo-spacing-4xl) var(--leo-spacing-2xl);
    border-start-end-radius: var(--leo-radius-xxl);
    border-end-end-radius: var(--leo-radius-xxl);
  }

  .message.end {
    --recap-message-from: calc(100% * var(--recap-inline));
    animation-delay: var(--leo-duration-xs);
    margin-inline-start: var(--leo-spacing-2xl);
    padding-inline: var(--leo-spacing-2xl) var(--leo-spacing-4xl);
    border-start-start-radius: var(--leo-radius-xxl);
    border-end-start-radius: var(--leo-radius-xxl);
    text-align: end;
  }

  /* After navigation, messages wait for the slide to land, so the two motions
   * don't add up. */
  @media (prefers-reduced-motion: no-preference) {
    .slide-frame[data-motion='enter'] & .message {
      animation-delay: var(--leo-duration-l);
    }

    .slide-frame[data-motion='enter'] & .message.end {
      animation-delay: calc(var(--leo-duration-l) + var(--leo-duration-xs));
    }
  }

  @media ${LANDSCAPE_QUERY} {
    .intro {
      padding-inline: var(--leo-spacing-2xl);
    }

    /* The intro messages become a pair of panels, like the stat slides'. */
    .messages {
      gap: var(--leo-spacing-s);
      padding-inline-end: var(--leo-spacing-2xl);
    }

    .message {
      animation-name: recap-panel-in;
    }

    .message.start {
      --recap-panel-from: calc(-1 * var(--leo-spacing-l));
      margin-inline: 0;
      padding: var(--leo-spacing-2xl);
      border-radius: var(--leo-radius-xxl) var(--leo-radius-xxl)
        var(--leo-radius-m) var(--leo-radius-m);
    }

    .message.end {
      --recap-panel-from: var(--leo-spacing-l);
      margin-inline: 0;
      padding: var(--leo-spacing-xl) var(--leo-spacing-2xl);
      border-radius: var(--leo-radius-m) var(--leo-radius-m) var(--leo-radius-xxl)
        var(--leo-radius-xxl);
      text-align: start;
    }
  }

  @media (prefers-reduced-motion: reduce) {
    .message {
      animation: none;
    }
  }
`
