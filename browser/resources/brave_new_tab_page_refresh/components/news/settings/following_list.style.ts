/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    padding: 24px;
    width: 100%;
  }

  h4 {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    margin-bottom: 16px;
  }

  .source-count {
    font: ${font.small.regular};
  }

  .sources {
    --leo-icon-size: 20px;

    display: flex;
    flex-direction: column;

    > * {
      display: flex;
      align-items: center;
      gap: 12px;
      padding: 4px 0;
    }

    .name {
      flex: 1 1 auto;
      font: ${font.default.semibold};
      margin-inline-end: 8px
    }

    img {
      height: var(--leo-icon-size);
      width: var(--leo-icon-size);
      border-radius: 4px;
    }

    button {
      font: ${font.components.buttonSmall};
      opacity: 0;
      transition: opacity 120ms;
    }

    > *:hover button {
      opacity: .7;
      &:hover {
        opacity: 1;
      }
    }
  }
`
