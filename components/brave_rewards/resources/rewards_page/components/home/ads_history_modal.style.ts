/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { css, scopedCSS } from '../../lib/scoped_css'

export const style = scopedCSS('ads-history-modal', css`
  & {
    overflow: auto;
    max-width: 600px;
    min-height: 275px;
    display: flex;
    flex-direction: column;
    gap: 16px;
    font: ${font.default.regular};

    @container style(--is-wide-view) {
      min-width: 400px;
    }
  }

  .items {
    margin-top: 4px;
    padding: 16px;
    border-radius: 12px;
    display: flex;
    flex-direction: column;
    border: solid 1px ${color.divider.subtle};
  }

  .item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 24px;
    padding: 8px 0;
    border-bottom: solid 1px ${color.divider.subtle};

    &:last-child {
      border-bottom: none;
    }
  }

  .ad-info {
    overflow: hidden;
  }

  .name {
    font: ${font.default.semibold};
  }

  .single-line {
    overflow: hidden;
    white-space: nowrap;
    text-overflow: ellipsis;
  }

  .domain {
    color: ${color.text.tertiary};
  }

  .actions {
    --leo-icon-size: 24px;
    display: flex;
    gap: 12px;

    button.on {
      --leo-icon-color: ${color.icon.interactive};
    }
  }

  leo-menu-item {
    --leo-icon-size: 16px;
    display: flex;
    gap: 8px;
    align-items: center;
  }
`)
