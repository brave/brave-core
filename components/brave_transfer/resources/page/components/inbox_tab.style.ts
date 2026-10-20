/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: ${spacing.m};
  }

  .header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: ${spacing.m};
  }

  h2 {
    font: ${font.heading.h4};
  }

  .table {
    display: grid;
    grid-template-columns: auto minmax(100px, 160px) auto 1fr;
    border-radius: ${radius.l};
    border: 1px solid ${color.divider.subtle};
    background: ${color.container.background};
    overflow: hidden;
  }

  .row {
    display: grid;
    grid-column: 1 / -1;
    grid-template-columns: subgrid;
    align-items: start;
    column-gap: ${spacing.xl};
    padding: ${spacing.l} ${spacing.xl};
  }

  .row + .row {
    border-top: 1px solid ${color.divider.subtle};
  }

  .header-row {
    padding-block: ${spacing.m};
    font: ${font.components.tableheader};
    color: ${color.text.tertiary};
    text-transform: uppercase;
  }

  .cell {
    min-width: 0;
  }

  .time {
    white-space: nowrap;
    color: ${color.text.secondary};
    font: ${font.small.regular};
    padding-top: 2px;
  }

  .from {
    font: ${font.default.semibold};
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .kind {
    display: flex;
    align-items: center;
    gap: ${spacing.s};
    color: ${color.text.secondary};
    font: ${font.small.regular};
    --leo-icon-size: 18px;
    --leo-icon-color: ${color.icon.default};
  }

  .content {
    display: flex;
    flex-direction: column;
    gap: ${spacing.s};
  }

  .text {
    white-space: pre-wrap;
    overflow-wrap: anywhere;
  }

  .text[data-collapsed=true] {
    display: -webkit-box;
    -webkit-box-orient: vertical;
    -webkit-line-clamp: 3;
    overflow: hidden;
  }

  .link {
    color: ${color.text.interactive};
    overflow-wrap: anywhere;
  }

  .file {
    display: flex;
    align-items: center;
    gap: ${spacing.m};
    min-width: 0;
  }

  .thumbnail,
  .file-icon {
    flex: 0 0 auto;
    width: 48px;
    height: 48px;
    border-radius: ${radius.m};
    background: ${color.container.highlight};
    object-fit: cover;
  }

  .file-icon {
    display: flex;
    align-items: center;
    justify-content: center;
    --leo-icon-size: 24px;
    --leo-icon-color: ${color.icon.secondary};
  }

  .file-info {
    display: flex;
    flex-direction: column;
    min-width: 0;
  }

  .file-name {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .file-size {
    color: ${color.text.tertiary};
    font: ${font.small.regular};
  }

  .item-warning,
  .item-note {
    display: flex;
    align-items: center;
    gap: ${spacing.s};
    font: ${font.small.regular};
    --leo-icon-size: 16px;
  }

  .item-warning {
    color: ${color.systemfeedback.warningText};
    --leo-icon-color: ${color.systemfeedback.warningIcon};
  }

  .item-note {
    color: ${color.text.secondary};
  }

  .item-actions {
    display: flex;
    flex-wrap: wrap;
    gap: ${spacing.s};
    margin-inline-start: -${spacing.m};
  }

  .load-more {
    display: flex;
    justify-content: center;
  }

  @media (max-width: 640px) {
    .table {
      display: flex;
      flex-direction: column;
    }

    .row.header-row {
      display: none;
    }

    .row {
      display: grid;
      grid-template-columns: 1fr auto;
      grid-template-areas:
        'from time'
        'kind kind'
        'content content';
      row-gap: ${spacing.s};
      padding: ${spacing.l};
    }

    .header-row + .row {
      border-top: none;
    }

    .time {
      grid-area: time;
    }

    .from {
      grid-area: from;
    }

    .kind {
      grid-area: kind;
    }

    .content {
      grid-area: content;
    }
  }
`
