/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .balance {
    border-radius: 12px;
    padding: 16px;
    background: ${color.container.highlight};
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .bat-icon {
    --leo-icon-size: 32px;
    flex: 0 0 auto;
  }

  .balance-header {
    font: ${font.default.regular};
    color: ${color.text.tertiary};
    display: flex;
    align-items: center;
    gap: 2px;

    .icon {
      height: 16px;
      width: auto;
      margin-left: 4px;
    }

    .provider-name {
      font: ${font.small.semibold};
      color: ${color.text.primary};
    }
  }

  .balance-value {
    font: ${font.default.semibold};
  }

  .form-header {
    font: ${font.default.semibold};
    margin-bottom: 8px;
  }

  .options {
    padding: 4px 16px;
    border: solid 1px ${color.divider.subtle};
    border-radius: 12px;
  }

  .option {
    padding: 12px 0;
    font: ${font.default.regular};
    display: flex;
    align-items: center;
    gap: 12px;
    border-bottom: solid 1px ${color.divider.subtle};
    min-height: 60px;

    &:last-child {
      border-bottom: none;
    }

    .exchange {
      color: ${color.text.tertiary};
      text-align: end;
      flex: 1 1 auto;
    }

    .custom-input {
      position: relative;

      &:before {
        content: 'BAT';
        display: block;
        position: absolute;
        inset: 7px 9px auto auto;
        color: ${color.text.tertiary};
      }
    }

    input {
      padding: 6px 38px 6px 8px;
      border-radius: 8px;
      border: solid 1px ${color.divider.strong};
      background: ${color.container.background};
      width: 120px;
      font: ${font.default.regular};
      color: ${color.text.primary};
    }
  }

  .recurring {
    display: flex;
    justify-content: space-between;

    .frequency {
      font: ${font.components.buttonDefault};
      color: ${color.text.interactive};
    }
  }

  .reconnect {
    --leo-icon-color: ${color.systemfeedback.warningIcon};
    --leo-icon-size: 32px;

    padding: 24px;
    border-radius: 8px;
    background: ${color.systemfeedback.warningBackground};
    color: ${color.systemfeedback.warningText};
    display: flex;
    gap: 16px;

    .content {
      flex: 1 1 auto;
      display: flex;
      flex-direction: column;
      gap: 4px;
    }
  }

  .actions {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }
`
