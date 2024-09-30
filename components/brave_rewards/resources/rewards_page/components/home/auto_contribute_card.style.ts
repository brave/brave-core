/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  h4 {
    --leo-toggle-height: 24px;
    --leo-toggle-width: 40px;

    padding: 16px;
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  .info {
    --leo-icon-size: 24px;

    padding: 16px;
    border: solid 1px ${color.divider.subtle};
    border-radius: 8px;

    .title {
      display: flex;
      align-items: center;
      gap: 16px;
      font: ${font.default.semibold};
      cursor: pointer;

      .text {
        flex: 1 1 auto;
      }
    }

    p {
      font: ${font.small.regular};
      color: ${color.text.tertiary};
      margin-top: 9px;
      margin-left: 40px;
    }
  }

  section {
    padding: 8px;
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .settings {
    border-radius: 8px;
    background: ${color.page.background};
    padding: 4px 16px;

    > * {
      display: flex;
      align-items: center;
      gap: 16px;
      justify-content: space-between;
      padding: 12px 0;
      border-bottom: solid 1px ${color.divider.subtle};
    }

    > :last-child {
      border-bottom: none;
    }

    label {
      color: ${color.text.tertiary};
    }

    .value {
      font: ${font.components.buttonDefault};
    }
  }

  .site-list {
    .heading {
      font: ${font.small.regular};
      color: ${color.text.tertiary};
      padding: 0 40px 0 8px;
      display: flex;
      gap: 8px;
      justify-content: space-between;
    }

    .site {
      --leo-icon-size: 18px;

      padding: 8px;
      display: flex;
      gap: 16px;
      align-items: center;
      font: ${font.default.semibold};

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
          height: 32px;
          width: 32px;
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
    }

    .show-all {
      text-align: center;
    }
  }

  .empty {
    text-align: center;
    margin: 16px 0;
    color: ${color.text.secondary};
  }

  .disabled {
    --leo-icon-size: 24px;
    padding: 24px 0;
    display: flex;
    flex-direction: column;
    gap: 8px;
    align-items: center;
    text-align: center;

    leo-icon {
      margin-bottom: 8px;
    }

    .title {
      font: ${font.default.semibold};
    }

    .text {
      color: ${color.text.tertiary};
    }
  }
`
