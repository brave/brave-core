/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'
import { color, effect, font } from '@brave/leo/tokens/css/variables'

export const style = scoped.css`
  & {
    background: ${color.neutral['10']};
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 12px;
    border-bottom: solid 1px ${color.divider.subtle};
  }

  &[data-shields-off='true'] {
    background: ${color.neutral['20']};
    color: ${color.text.secondary};

    .site-icon img {
      filter: grayscale(100%);
    }
  }

  .site-info {
    --leo-toggle-width: 65px;
    --leo-toggle-height: 40px;

    padding: 12px;
    display: flex;
    align-items: center;
    gap: 16px;
  }

  .site-text {
    flex: 1 1 auto;
    overflow: hidden;
  }

  h3 {
    text-overflow: ellipsis;
    white-space: nowrap;
    overflow: hidden;
  }

  .site-icon {
    padding: 6px;
    width: 32px;
    height: 32px;
    border-radius: 50%;
    background: ${color.container.background};
    box-shadow: ${effect.elevation['01']};

    img {
      display: block;
      width: 100%;
      height: 100%;
    }
  }

  .shields-status {
    font: ${font.large.regular};

    strong {
      font: ${font.large.semibold};
    }
  }

  .block-info {
    border-radius: 16px;
    padding: 12px 8px;
    background: ${color.container.background};
    box-shadow: ${effect.elevation['01']};
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;

    .count {
      font: ${font.heading.h2};
    }
  }

  .report-prompt {
    padding: 12px 16px;
    border-radius: 16px;
    background: ${color.neutral['5']};
    box-shadow: ${effect.elevation['01']};
    display: flex;
    align-items: center;
    gap: 12px;

    leo-button {
      margin-left: auto;
      flex: 0 1 auto;
    }
  }
`
