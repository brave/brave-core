/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  section {
    padding: 8px 12px;
  }

  .counter {
    padding: 24px 16px 16px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 20px;

    img {
      width: auto;
      height: 87px;
    }
  }

  .counter-text {
    font: ${font.default.regular};
    color: ${color.text.tertiary};
    text-align: center;
    min-height: 80px;
  }

  .counter-value {
    --leo-icon-size: 16px;
    font: ${font.heading.h2};
    color: ${color.text.primary};

    leo-tooltip {
      padding-inline-start: 8px;
    }
  }

  .earnings-range {
    font: ${font.default.semibold};
  }

  .connect {
    display: flex;
    align-items: center;
    gap: 8px;

    leo-button {
      flex: 0 0 auto;
    }
  }

  .connect-text {
    flex: 1 1 auto;
    color: ${color.text.primary};
    font: ${font.default.semibold};
  }

  .connect-subtext {
    font: ${font.small.regular};
    opacity: 0.6;
  }

  .ads-summary {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .ads-summary-title {
    font: ${font.default.semibold};
    cursor: pointer;
    display: flex;
    align-items: center;

    .value {
      background:
        linear-gradient(314deg, #3C823C 8.49%, #2ABA32 48.32%, #94DF91 99.51%);
      background-clip: text;
      -webkit-text-fill-color: transparent;
      padding-inline-end: 4px;
    }

    > :first-child {
      flex: 1 0 auto;
    }
  }

  .ads-summary-nav {
    --leo-icon-size: 16px;

    display: flex;
    justify-content: space-between;

    button {
      font: ${font.components.buttonSmall};
      color: ${color.text.secondary};
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 4px 0;
    }
  }

  .warning-box {
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.systemfeedback.warningIcon};

    color: ${color.systemfeedback.warningText};
    background: ${color.systemfeedback.warningBackground};
    padding: 16px;
    border-radius: 12px;
    font: ${font.default.regular};
    display: flex;
    gap: 16px;
    align-items: center;
  }
`
