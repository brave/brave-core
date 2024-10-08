/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../lib/scoped_css'

export const style = scoped.css`
  & {
    margin: 0 auto;
    padding: 72px 32px 16px;
    max-width: 620px;
    color: ${color.text.primary};
  }

  .brave-rewards-logo {
    position: absolute;
    inset-block-start: 16px;
    inset-inline-start: 32px;
    display: inline-block;
  }

  nav {
    --leo-button-radius: 50%;
    text-align: left;
  }

  h1 {
    font: ${font.heading.h2};
    margin: 36px 0 16px;
    text-align: center;
  }

  .text {
    color: ${color.text.tertiary};
    text-align: center;
    margin-bottom: 32px;
  }

  section {
    border-radius: 16px;
    padding: 0 24px;
    background: ${color.container.background};
    display: flex;
    flex-direction: column;

    button {
      --leo-icon-size: 32px;

      position: relative;
      display: flex;
      align-items: center;
      gap: 11px;
      text-align: left;
      padding-left: 10px;
      height: 84px;
      border-top: solid 1px ${color.divider.subtle};
      font: ${font.large.semibold};

      &:first-of-type {
        border-top: none;
      }

      &:disabled {
        --provider-icon-color: ${color.icon.disabled};
        color: ${color.text.tertiary};
        cursor: default;
      }

      .icon {
        width: 32px;
        height: auto;
      }

      .name {
        flex: 1 0 auto;
      }

      .message {
        display: block;
        color: ${color.text.tertiary};
        font: ${font.small.regular};
      }

      .caret {
        --leo-icon-size: 24px;
        --leo-progressring-size: 24px;

        font: ${font.components.buttonSmall};
        color: ${color.icon.interactive};
        display: flex;
        align-items: center;
      }
    }
  }

  .new-badge {
    position: absolute;
    top: 10px;
    left: -14px;
    padding: 4px;
    border-radius: 4px;
    background: ${color.primary['20']};
    color: ${color.primary['50']};
    font: ${font.components.label};
  }

  .regions-learn-more {
    margin-top: 8px;
    color: ${color.text.tertiary};
    font: ${font.small.link};

    a {
      color: inherit;
    }
  }

  .connect-error {
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.systemfeedback.warningIcon};

    margin-top: 24px;
    background: ${color.systemfeedback.warningBackground};
    color: ${color.systemfeedback.warningText};
    border-radius: 16px;
    padding: 16px;
    display: flex;
    gap: 16px;
    align-items: center;
  }

  .self-custody-note {
    margin-top: 24px;
    font: ${font.small.regular};
    color: ${color.text.secondary};
  }

  .self-custody-learn-more {
    margin-top: 8px;

    a {
      font: ${font.components.buttonDefault};
      color: ${color.text.interactive};
      text-decoration: none;
    }
  }

  h3 {
    --leo-icon-size: 18px;

    display: flex;
    align-items: center;
    gap: 4px;
    color: ${color.text.secondary};
    font: ${font.default.regular};
    margin: 32px 0 8px 0;

    leo-tooltip [slot=content] {
      max-width: 300px;
    }
  }
`
