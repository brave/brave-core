/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 24px;
  }

  .text {
    --leo-icon-size: 40px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    text-align: center;
  }

  .methods {
    padding: 0 16px;
    display: flex;
    flex-direction: column;
    border-radius: 12px;
    border: solid 1px ${color.divider.subtle};
  }

  .method {
    padding: 12px 0;
    border-bottom: solid 1px ${color.divider.subtle};
    font: ${font.large.semibold};
    display: flex;
    flex-direction: column;

    &:last-child {
      border-bottom: none;
    }

    > * {
      display: flex;
      gap: 12px;
      align-items: center;
    }
  }

  .provider-icon {
    padding: 8px;
    border-radius: 8px;
    background: ${color.page.background};
    flex: 0 0 auto;

    .icon {
      width: 24px;
      height: auto;
      display: block;
    }
  }

  .provider-text {
    .subtext {
      font: ${font.small.regular};
      color: ${color.text.tertiary};
    }
  }

  .actions {
    display: flex;
    flex-direction: column;
    gap: 4px;
    text-align: center;

    a {
      padding: 12px;
      font: ${font.components.buttonDefault};
      color: ${color.text.interactive};
      text-decoration: none;
    }
  }
`
