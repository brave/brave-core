/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    flex: 0 1 540px;
    align-self: stretch;
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .feed-card {
    background: rgba(255, 255, 255, 0.05);
    border-radius: 16px;
    padding: 12px 16px 16px;

    &:empty {
      display: none;
    }
  }

  .caught-up {
    --leo-icon-size: 24px;

    display: flex;
    align-items: center;
    gap: 16px;
    color: rgba(255, 255, 255, 0.5);
    font: ${font.small.regular};
    padding: 16px 0;

    hr {
      flex: 1 1 auto;
      border-color: rgba(255, 255, 255, 0.1);
    }

    p {
      display: flex;
      align-items: center;
      gap: 6px;
    }
  }

  .scroll-reset {
    height: var(--news-feed-control-bar-height);
    position: absolute;
    visibility: hidden;
  }

  .loading {
    --leo-progressring-color: rgba(255, 255, 255, 0.25);
    --leo-progressring-size: 32px;

    display: flex;
    justify-content: center;
    position: sticky;
    inset-block-start: calc(16px + var(--news-feed-control-bar-height));

    > * {
      margin: 16px 0;
    }
  }
`
