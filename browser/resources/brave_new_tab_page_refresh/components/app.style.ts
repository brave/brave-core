/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../lib/scoped_css'

export const style = scoped.css`
  & {
    --search-transition-duration: 120ms;
  }

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

  .clock {
    font: ${font.large.semibold};
    color: #fff;
    opacity: .8;
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
    container-type: inline-size;
    position: relative;
    z-index: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 100vh;
    gap: 16px;
    padding: 16px 24px;

    > * {
      transition:
        opacity var(--search-transition-duration),
        transform var(--search-transition-duration);

      .search-box-expanded & {
        opacity: 0;
        transform: scale(0.9);
      }
    }
  }

  .topsites-container {
    padding: 16px 0;
    align-self: stretch;
    display: flex;
    gap: 16px;
  }

  .searchbox-container {
    align-self: stretch;

    .search-box-expanded & {
      opacity: 1;
      transform: none;
    }
  }

  .spacer {
    flex: 1 1 auto;
    align-self: stretch;
  }

  .widget-container {
    --widget-height: 128px;
    --widget-width: 420px;
    --widget-flex-basis: var(--widget-width);

    align-self: stretch;
    flex: 0 0 var(--widget-height);

    display: flex;
    justify-content: center;
    align-items: stretch;
    gap: 16px;

    &:empty {
      flex-basis: 0;
    }

    @container (max-width: 900px) {
      --widget-flex-basis: var(--widget-height);

      width: var(--widget-width);
      align-self: center;
      flex-basis: auto;
      display: flex;
      flex-direction: column;
    }
  }

`

style.passthrough.css`
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

  dialog, [popover] {
    border: none;
    color: inherit;
    margin: 0;
    padding: 0;
    background: none;

    &::backdrop {
      background-color: transparent;
    }
  }

  .popover-menu {
    padding: 4px;
    border-radius: 8px;
    border: solid 1px ${color.divider.subtle};
    background: ${color.container.background};
    box-shadow: 0 1px 0 0 rgba(0, 0, 0, 0.05);
    display: flex;
    flex-direction: column;
    gap: 4px;
    min-width: 180px;

    .divider {
      height: 1px;
      background: ${color.divider.subtle};
    }

    button {
      --leo-icon-size: 20px;
      padding: 8px 24px 8px 8px;
      border-radius: 4px;
      display: flex;
      align-items: center;
      gap: 16px;

      &:hover, &.highlight {
        background: ${color.container.highlight};
      }
    }
  }
`
