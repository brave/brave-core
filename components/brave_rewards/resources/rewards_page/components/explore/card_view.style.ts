/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  h4 {
    display: flex;
    align-items: center;
    justify-content: space-between;

    &:empty {
      display: none;
    }
  }

  .banner {
    display: flex;
    border-radius: 12px;
    overflow: hidden;

    img {
      width: 100%;
      height: auto;
    }
  }

  section:empty {
    display: none;
  }

  section a {
    display: flex;
    gap: 16px;
    text-decoration: none;
    color: ${color.text.primary};
    padding: 8px;
    border-bottom: solid 1px ${color.divider.subtle};

    &:last-child {
      border-bottom: none;
    }
  }

  .thumbnail {
    flex: 0 0 56px;
    height: 56px;
    width: 56px;
    overflow: hidden;
    border-radius: 12px;
    border: solid 1px ${color.divider.subtle};
    display: flex;
    align-items: center;
    justify-content: center;

    img {
      width: 100%;
      height: auto;
    }

    .favicon {
      width: 24px;
      height: auto;
      border-radius: 4px;
    }
  }

  .item-info {
    display: flex;
    flex-direction: column;

    .title {
      font: ${font.default.semibold};
    }

    .description {
      font: ${font.xSmall.regular};
      color: ${color.text.tertiary};
    }
  }
`
