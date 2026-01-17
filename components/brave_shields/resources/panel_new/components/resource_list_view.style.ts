/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  .list {
    --leo-icon-size: 20px;

    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .group {
    display: flex;
    flex-direction: column;
    gap: 8px;

    &[data-expanded=false] > .list {
      display: none;
    }

    & .list .resource {
      --self-branch-path-height: 40px;

      position: relative;

      leo-icon {
        visibility: hidden;
      }

      &::before {
        content: '';
        position: absolute;
        height: var(--self-branch-path-height);
        width: 10px;
        inset-block-start: calc(-1 * var(--self-branch-path-height) + 15px);
        inset-inline-start: 18px;
        border-block-end: solid 1px ${color.divider.strong};
        border-inline-start: solid 1px ${color.divider.strong};
        border-radius: 0 0 0 2px;
      }

      &:first-child {
        --self-branch-path-height: 24px;
      }
    }
  }

  .resource {
    font: ${font.small.regular};
    padding: 4px 8px;
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .text {
    flex: 1 1 auto;
    overflow: hidden;
    white-space: nowrap;
    text-overflow: ellipsis;
  }

  button.action {
    font: ${font.small.link};
    text-decoration: underline;
    color: ${color.text.tertiary};
  }
`
