/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

import newsGraphic from '../../assets/news_graphic.svg'

export const style = scoped.css`
  & {
    min-height: 100%;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .title {
    font: ${font.components.buttonSmall};
  }

  .opt-in {
    display: flex;
    align-items: center;
    gap: 12px;

    .graphic {
      background-image: url(${newsGraphic});
      background-repeat: no-repeat;
      background-size: contain;
      background-position: center center;
      width: 56px;
      align-self: stretch;
    }

    .text {
      flex: 1 1 auto;
      font: ${font.default.semibold};
      color: ${color.text.primary};
    }

    .actions {
      padding: 0 8px;

      leo-button {
        --leo-button-color: rgba(255, 255, 255, 0.20);
        white-space: nowrap;
      }
    }
  }

  .peek {
    flex: 1 1 auto;
    display: flex;
    align-items: center;
    gap: 12px;
    text-decoration: none;

    img {
      min-width: 71px;
      width: 71px;
      height: 48px;
      object-fit: cover;
      object-position: center top;
      border-radius: 4px;
      opacity: 0;
      transition: opacity 400ms ease-in-out;
      &.loaded {
        opacity: 1;
      }
    }

    .meta {
      --leo-icon-size: 12px;

      font: ${font.xSmall.regular};
      color: ${color.text.primary};
      display: flex;
      align-items: center;
      gap: 6px;
    }

    .item-title {
      margin-top: 4px;
      font: ${font.small.semibold};
      color: #fff;
      overflow: hidden;
      line-clamp: 2;
      display: -webkit-box;
      -webkit-box-orient: vertical;
      -webkit-line-clamp: 2;
    }
  }

  .loading {
    .img {
      width: 71px;
      height: 48px;
      border-radius: 4px;
    }

    .text {
      flex: 1 1 auto;
    }

    .meta {
      height: 1em;
      width: 100px;
      border-radius: 4px;
    }

    .item-title {
      height: 2.8em;
      width: 100%;
      border-radius: 4px;
    }
  }
`
