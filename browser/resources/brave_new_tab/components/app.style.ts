/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped, global } from '../lib/scoped_css'

export const style = scoped.css`

  .settings {
    --leo-icon-size: 20px;

    position: absolute;
    inset-block-start: 4px;
    inset-inline-end: 4px;
    block-size: 20px;
    inline-size: 20px;
    opacity: 0.5;
    color: #fff;
    filter: drop-shadow(0px 1px 4px rgba(0, 0, 0, 0.60));

    &:hover {
      opacity: 0.7;
      cursor: pointer;
    }
  }

  main {
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 100vh;
    padding-top: 40px;
  }

  .topsites-container {
    min-height: 32px;
  }

  .searchbox-container {
    flex: 1 1 auto;
    margin: 16px 0;
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
  }
`
