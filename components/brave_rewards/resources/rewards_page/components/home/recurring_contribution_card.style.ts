/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  h4 {
    padding: 16px;
  }

  section {
    padding: 8px;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .list {
    > * {
      padding: 8px 0;
      border-bottom: solid 1px ${color.divider.subtle};
      font: ${font.default.semibold};
      display: flex;
      gap: 16px;
      align-items: center;
    }

    > :first-child {
      padding-top: 0;
    }

    > :last-child {
      padding-bottom: 0;
      border-bottom: none;
    }

    a {
      color: inherit;
      text-decoration: none;
    }

    .icon {
      border-radius: 8px;
      background: ${color.page.background};
      padding: 4px;

      img {
        display: block;
        width: 32px;
        height: 32px;
      }
    }

    .name {
      flex: 1 1 auto;

      a {
        --leo-icon-size: 12px;
        display: flex;
        gap: 4px;
        align-items: center;
      }
    }

    .next-contribution {
      --leo-icon-size: 12px;
      font: ${font.xSmall.regular};
      color: ${color.text.tertiary};
      display: flex;
      gap: 8px;
      align-items: center;
    }

    .amount {
      color: ${color.text.secondary};
      text-align: right;
    }

    .exchange-amount {
      font: ${font.xSmall.regular};
      color: ${color.text.tertiary};
    }

    .more {
      --leo-icon-size: 24px;
    }

    leo-menu-item {
      --leo-icon-size: 20px;

      font: ${font.default.regular};
      display: flex;
      align-items: center;
      gap: 16px;
    }
  }

  .empty {
    margin: 24px 0;
    text-align: center;
    color: ${color.text.secondary};
  }

  .show-all {
    text-align: center;
  }
`
