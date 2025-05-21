/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    color: #fff;

    font: ${font.small.semibold};
    display: flex;
    flex-direction: column;
    gap: 0;
  }

  button {
    border-radius: 8px;
    padding: 8px 8px 8px 34px;

    &:hover {
      opacity: 0.7;
    }

    &.selected {
      background: rgba(255, 255, 255, 0.1);
    }
  }

  details[open] leo-icon {
    transform: rotate(90deg);
  }

  details.empty summary > leo-icon {
    visibility: hidden;
  }

  summary {
    --leo-icon-size: 18px;
    --leo-button-padding: 2px;
    --leo-button-radius: 4px;

    padding: 8px;
    display: flex;
    align-items: center;
    gap: 8px;
    cursor: pointer;
    user-select: none;

    > :last-child {
      margin-left: auto;
      flex: 0 1 auto;
    }
  }

  .group-items {
    display: flex;
    flex-direction: column;
    gap: 0;
    font: ${font.small.regular};

    & button.show-all-button {
      display: block;
      color: rgba(255, 255, 255, 0.5);
    }
  }

`
