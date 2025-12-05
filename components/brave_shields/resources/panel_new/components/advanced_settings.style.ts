/* Copyright (c) 2025 The Brave Authors. All rights reserved.
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

      > button {
        border-radius: 8px;
      }

      > button:hover {
        background: ${color.button.hover};
      }

      > button:disabled {
        visibility: hidden;
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

  select {
    flex: 1 1 auto;
    appearance: base-select;
    border: solid 1px ${color.divider.subtle};
    border-radius: 8px;
    padding: 11px 8px;
    background: ${color.container.background};

    &:hover {
      box-shadow: 0px 1px 2px -1px rgba(0, 0, 0, .08), 0px 1px 3px 0px rgba(0, 0, 0, .08);
      border-color: ${color.divider.strong};
    }

    &::picker-icon {
      display: none;
    }

    > button {
      width: 100%;
      display: flex;
      align-items: center;
      gap: 8px;

      leo-icon {
        margin-inline-start: auto;
      }
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
    padding: 11px 8px;

    button {
      color: ${color.text.interactive};
      font: ${font.small.regular};
      text-decoration: underline;
    }
  }
`
