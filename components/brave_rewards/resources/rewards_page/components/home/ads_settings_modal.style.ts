/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    overflow: auto;
    display: flex;
    flex-direction: column;
    gap: 8px;
    font: ${font.default.regular};

    @container style(--is-wide-view) {
      max-width: 600px;
    }
  }

  .description {
    margin-bottom: 16px;
    color: ${color.text.tertiary};
  }

  section {
    background: ${color.page.background};
    border-radius: 8px;
  }

  .summary {
    .row {
      display: flex;
      gap: 8px;
      align-items: center;
      justify-content: space-between;
      padding: 16px;
      border-bottom: solid 1px ${color.container.background};

      .value {
        font: ${font.default.semibold};
      }
    }
  }

  .ad-types {
    padding: 12px 16px;

    .header {
      margin-bottom: 16px;
      display: flex;
      gap: 8px;
      align-items: center;
      justify-content: space-between;
      font: ${font.small.semibold};
      color: ${color.text.tertiary};
    }

    .row {
      --leo-toggle-width: 40px;
      --leo-toggle-height: 24px;

      margin: 12px 0;
      display: flex;
      gap: 8px;
      align-items: center;
      padding: 8px 0;
    }

    .name {
      --leo-icon-size: 16px;

      flex: 1 0 auto;
      display: flex;
      gap: 8px;
      align-items: center;

      leo-tooltip [slot=content] {
        display: flex;
        max-width: 300px;
        flex-direction: column;
        gap: 8px;
      }
    }
  }

  .subdivisions {
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding: 16px;

    label {
      font: ${font.default.semibold};
    }
  }

  .subdivision-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 8px;
  }
`
