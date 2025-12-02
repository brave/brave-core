/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    overflow: auto;
    min-height: 275px;
    display: flex;
    flex-direction: column;
    gap: 16px;
    font: ${font.default.regular};

    @container style(--is-wide-view) {
      width: 530px;
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

    position: relative;
    display: flex;
    gap: 12px;

    button {
      padding: 4px;
      border-radius: 8px;
      border: solid 1px transparent;
    }

    button:hover {
      --leo-icon-color: ${color.text.interactive};
    }

    button.on {
      --leo-icon-color: ${color.icon.interactive};

      background: ${color.container.interactive};
      border-color:
        color-mix(in srgb, ${color.text.interactive} 60%, transparent);
    }
  }

  .more-menu {
    position: absolute;
    top: calc(100% + 4px);
    right: 0;
    z-index: 1;

    padding: 4px;
    border-radius: 8px;
    border: solid 1px ${color.divider.subtle};
    background: ${color.container.background};
    box-shadow: 0 1px 0 0 rgba(0, 0, 0, 0.05);
    display: flex;
    flex-direction: column;
    gap: 4px;
    min-width: 180px;

    button {
      --leo-icon-size: 20px;

      padding: 8px;
      border-radius: 4px;
      display: flex;
      align-items: center;
      gap: 16px;
      white-space: nowrap;

      &:hover, &.highlight {
        background: ${color.container.highlight};
      }
    }
  }
`
