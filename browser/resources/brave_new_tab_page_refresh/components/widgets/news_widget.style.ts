/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

import newsPreviewImage from '../../assets/news_preview.svg'

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

  .peek {
    flex: 1 1 auto;
    display: flex;
    align-items: center;
    gap: 12px;
    text-decoration: none;

    .article-image {
      min-width: 71px;
      width: 71px;
      height: 48px;
      object-fit: cover;
      object-position: center top;
      border-radius: 4px;
    }

    img {
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

  .preview .article-image {
    background-image: url(${newsPreviewImage});
    background-repeat: no-repeat;
    background-size: contain;
    background-position: center center;
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
