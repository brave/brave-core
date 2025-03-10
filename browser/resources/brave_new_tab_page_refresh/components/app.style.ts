/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped, global } from '../lib/scoped_css'

export const style = scoped.css`
  .top-controls {
    position: absolute;
    inset-block-start: 4px;
    inset-inline-end: 4px;
    min-height: 24px;
    display: flex;
    gap: 8px;
    align-items: center;
    z-index: 2;
  }

  .settings {
    --leo-icon-size: 20px;

    opacity: 0.5;
    color: #fff;
    filter: drop-shadow(0px 1px 4px rgba(0, 0, 0, 0.60));

    &:hover {
      opacity: 0.7;
      cursor: pointer;
    }
  }

  .allow-background-pointer-events {
    /* This element will allow pointer events to target the background. */
    pointer-events: none;

    /* But children will not (unless they explicitly allow it). */
    > :not(.allow-background-pointer-events) {
      pointer-events: auto;
    }

    /* And not when a popover is open. When a popover is open and the background
       contains an interactive iframe, pointer events on a background iframe
       will not "light-dismiss" the popover. */
    :scope:has(:popover-open) & {
      pointer-events: auto;
    }
  }

  main {
    position: relative;
    z-index: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 100vh;
    gap: 16px;
    padding: 16px 24px;
  }

  .spacer {
    flex: 1 1 auto;
    align-self: stretch;
  }
`

global.css`
  @scope (${style.selector}) {

    & {
      font: ${font.default.regular};
      color: ${color.text.primary};
      interpolate-size: allow-keywords;
    }

    button {
      margin: 0;
      padding: 0;
      background: 0;
      border: none;
      text-align: unset;
      width: unset;
      font: inherit;
      cursor: pointer;

      &:disabled {
        cursor: default;
      }
    }

    h2 {
      font: ${font.heading.h2};
      margin: 0;
    }

    h3 {
      font: ${font.heading.h3};
      margin: 0;
    }

    h4 {
      font: ${font.heading.h4};
      margin: 0;
    }

    p {
      margin: 0;
    }
  }
`
