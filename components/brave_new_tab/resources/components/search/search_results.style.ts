/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect, font, gradient } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`

  & {
    border-radius: 16px;
    background: ${color.container.background};
    display: flex;
    flex-direction: column;
    overflow: clip;
    box-shadow: ${effect.elevation['01']};
  }

  .result-options {
    padding: 8px;
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  button {
    --leo-icon-size: 32px;

    border-radius: 8px;
    padding: 4px 8px;
    display: flex;
    align-items: center;
    gap: 12px;
    color: ${color.text.primary};
    text-decoration: none;

    &:hover, &.selected {
      background: ${color.neutral['10']};
    }
  }

  .result-image {
    flex: 0 0 32px;
    min-height: 32px;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  img {
    width: 32px;
    height: 32px;
    border-radius: 8px;

    &.icon {
      width: 24px;
      height: 24px;
      opacity: .7;
    }

    &.favicon {
      width: 20px;
      height: 20px;
    }
  }

  leo-icon {
    --leo-icon-size: 24px;

    width: 32px;
    height: 32px;
    border-radius: 8px;
    padding: 4px;

    &.brave-leo-icon {
      --leo-icon-color: #fff;
      background: ${gradient.iconsActive};
    }
  }

  .content {
    flex: 1 1 auto;
    display: flex;
    flex-direction: column;
    font: ${font.large.regular};
  }

  .description {
    font: ${font.small.regular};
    color: ${color.neutral['30']};
  }

  .suggestions-prompt {
    padding: 16px;
    display: flex;
    flex-direction: column;
    gap: 12px;
    background: ${color.container.interactive};

    h4 {
      font: ${font.default.semibold};
    }

    p {
      font: ${font.small.regular};
    }

    .actions {
      display: flex;
      align-items: center;
      gap: 8px;

      > * {
        flex: 0 1 auto;
      }
    }
  }

`
