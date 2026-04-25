/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`

  & {
    --leo-icon-size: 20px;

    padding: 8px;
    display: flex;
    flex-direction: column;
    gap: 8px;

    > * {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 0 4px 0 8px;

      > :last-child {
        padding: 4px;
        min-width: 28px;
        margin-inline-start: -4px;
      }

      > leo-button {
        border-radius: 8px;
        flex: 0;
      }

      > leo-button:hover {
        background: ${color.button.hover};
      }

      > leo-button[isdisabled=true] {
        visibility: hidden;
      }

      a {
        color: ${color.icon.default};
      }
    }
  }

  .toggle {
    flex: 1 1 auto;
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 11px 8px;

    & > *:last-child {
      margin-inline-start: auto;
    }
  }

  leo-dropdown {
    flex: 1 1 auto;

    [slot=value] {
      display: flex;
      width: 100%;
      align-items: center;
      gap: 8px;
    }
  }

  .counter {
    border-radius: 6px;
    padding: 2px 4px;
    background-color: ${color.neutral[10]};
    font: ${font.small.semibold};
    color: ${color.neutral[60]};
  }

  .block-scripts-text {
    > * {
      display: flex;
      align-items: center;
      gap: 8px;
    }

    .note {
      font: ${font.small.regular};
      color: ${color.text.tertiary};
    }
  }

  .block-elements {
    flex: 1 1 auto;
    padding: 11px 8px;

    button {
      color: ${color.text.interactive};
      font: ${font.small.regular};
      text-decoration: underline;
    }
  }
`
