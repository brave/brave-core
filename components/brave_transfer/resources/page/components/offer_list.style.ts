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

  h2 {
    font: ${font.heading.h4};
  }

  .offer {
    display: flex;
    flex-direction: column;
    gap: ${spacing.l};
    padding: ${spacing.xl};
    border-radius: ${radius.l};
    border: 2px solid ${color.primary[30]};
    background: ${color.container.background};
  }

  .offer-header {
    display: flex;
    flex-wrap: wrap;
    align-items: baseline;
    justify-content: space-between;
    gap: ${spacing.s};
  }

  .offer-from {
    font: ${font.large.regular};
  }

  .offer-expiry {
    font: ${font.small.regular};
    color: ${color.text.tertiary};
    font-variant-numeric: tabular-nums;
  }

  .offer-items {
    display: flex;
    flex-direction: column;
    gap: ${spacing.s};
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .offer-items li {
    display: flex;
    align-items: center;
    gap: ${spacing.m};
    min-width: 0;
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.icon.default};
  }

  .item-name {
    flex: 1 1 auto;
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .item-size {
    color: ${color.text.tertiary};
    font: ${font.small.regular};
  }

  .offer-actions {
    display: flex;
    justify-content: flex-end;
    gap: ${spacing.m};
  }

  @media (max-width: 600px) {
    .offer-actions > * {
      flex: 1 1 0;
    }
  }
`
