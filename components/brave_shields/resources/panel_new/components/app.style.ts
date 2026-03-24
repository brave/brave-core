/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'
import { color, font } from '@brave/leo/tokens/css/variables'

export const style = scoped.css`
  & {
    background: ${color.container.background};
    color-scheme: light dark;
  }
`

style.passthrough.css`
  /* Disable toggle animations (unless the user is hovering over the toggle) to
   * avoid toggle thumb jumping on first render. */
  --leo-toggle-transition-duration: 0;

  leo-toggle:hover {
    --leo-toggle-transition-duration: .12s;
  }

  & {
    font: ${font.default.regular};
    color: ${color.text.primary};
  }

  p {
    margin: 0;
  }

  h1 {
    font: ${font.heading.h1};
    margin: 0;
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

  button {
    margin: 0;
    padding: 0;
    background: none;
    border: none;
    text-align: unset;
    width: unset;
    font: inherit;
    cursor: pointer;

    &:disabled {
      cursor: default;
    }
  }

  main {
    display: flex;
    flex-direction: column;
    gap: 8px;
    max-height: calc(var(--browser-window-height, 9999px) - 112px);

    /* Drop shadow on previous sibling when scrollable area is scrolled */
    > :has(+ [data-scrolled]) {
      position: relative;

      &::after {
        content: '';
        position: absolute;
        display: block;
        top: 100%;
        width: 100%;
        height: 8px;
        background: linear-gradient(rgba(0, 0, 0, 0.06), transparent);
        pointer-events: none;
        z-index: 1;
      }
    }

    > .scrollable {
      overflow-y: auto;
      flex: 1 1 auto;
      margin-top: -8px;
      margin-bottom: -8px;
    }
  }

  .overflow-ellipsis-start {
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;

    &:dir(ltr) {
      direction: rtl;
      text-align: end;
    }
  }
`
