/* Copyright (c) 2024 The Brave Authors. All rights reserved.
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

  main {
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 100vh;
    padding-top: 40px;
  }

  .topsites-container {
    margin: 16px 24px;
    align-self: stretch;
    display: flex;
    gap: 16px;
  }

  .searchbox-container {
    flex: 1 1 auto;
    margin: 16px 0;
    align-self: stretch;
  }

  .background-caption-container {
    margin: 8px 0;
  }

  .widget-container {
    min-height: 8px;
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
  }
`
