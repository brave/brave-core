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
    gap: 8px;
  }

  .loading {
    --leo-progressring-size: 32px;

    padding-top: 33%;
    display: flex;
    align-items: center;
    justify-content: center;
    opacity: 1;

    transition: opacity 500ms 1.5s ease-in-out;

    @starting-style {
      opacity: 0;
    }
  }

  .columns {
    display: flex;
    gap: 24px;

    > * {
      flex: 1 1 50%;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
  }

  h4 {
    padding: 16px;
    display: flex;
    align-items: center;
    justify-content: space-between;

    a {
      --leo-icon-size: 16px;
      --leo-icon-color: ${color.icon.interactive};

      font: ${font.components.buttonSmall};
      color: ${color.text.interactive};
      text-decoration: none;
      display: flex;
      gap: 8px;
    }
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

    img {
      flex: 0 0 56px;
      height: 56px;
      width: 56px;
      border-radius: 12px;
      border: solid 1px ${color.divider.subtle};
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
