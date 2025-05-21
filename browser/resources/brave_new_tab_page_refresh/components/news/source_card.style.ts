/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 8px;
    font: ${font.default.semibold};
  }

  .cover {
    background: var(--cover-background, ${color.container.highlight});
    position: relative;
    border-radius: 8px;
    padding: 12px;
    height: 80px;
    overflow: hidden;
    box-shadow: rgba(99, 105, 110, 0.18) 0px 0px 8px 0px;
    display: flex;
    justify-content: center;
    align-items: center;

    img {
      height: 56px;
    }

    button {
      --leo-icon-size: 16px;

      position: absolute;
      inset-block-start: 8px;
      inset-inline-end: 8px;
      opacity: 1;
      transition: opacity 200ms, visibility 200ms allow-discrete;
      background: rgba(150, 150, 150, 0.8);
      border-radius: 8px;
      padding: 6px;
      color: #fff;

      &:hover {
        background: rgba(150, 150, 150, 1);
      }
    }

    &.enabled button {
      opacity: 0;
      visibility: hidden;
    }

    &:hover button {
      opacity: 1;
      visibility: visible;
    }
  }
`
