/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  a {
    text-decoration: none;
    color: ${color.text.primary};
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .hero img {
    margin-bottom: 12px;
    width: 100%;
    height: 269px;
    object-fit: cover;
    object-position: center top;
    border-radius: 6px;
  }

  .metadata {
    --leo-icon-size: 12px;

    padding: 6px 0;
    display: flex;
    align-items: center;
    gap: 4px;
    font: ${font.xSmall.regular};
    color: rgba(255, 255, 255, 0.5);
  }

  .actions {
    --leo-icon-size: 16px;

    flex: 1 1 auto;
    display: flex;
    justify-content: flex-end;

    &:hover {
      color: #fff;
    }
  }

  .article-menu-anchor {
    anchor-name: --news-article-menu-button;
  }

  .article-menu {
    position-anchor: --news-article-menu-button;
    position-area: block-end span-inline-start;
    position-try-fallbacks: flip-block;
  }

  .preview {
    font: ${font.default.semibold};
    color: #fff;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;

    img {
      min-width: 96px;
      width: 96px;
      height: 64px;
      object-fit: cover;
      object-position: center top;
      border-radius: 6px;
    }
  }
`
