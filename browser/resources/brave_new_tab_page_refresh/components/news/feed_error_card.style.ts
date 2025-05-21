/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

import connectionErrorImage from '../../assets/news_connection_error.svg'
import noArticlesImage from '../../assets/news_no_articles.svg'

export const style = scoped.css`
  & {
    text-align: center;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
    padding: 32px;
  }

  .graphic {
    height: 72px;
    width: 100%;
    background-image: url(${connectionErrorImage});
    background-repeat: no-repeat;
    background-position: center;
    background-size: contain;
    margin-bottom: 24px;
  }

  .no-articles.graphic {
    background-image: url(${noArticlesImage});
    height: 88px;
  }

  h3 {
    font: ${font.default.semibold};
    color: #fff;
  }

  leo-button {
    --leo-button-color: rgba(255, 255, 255, 0.1);
    border-radius: 20px;
    overflow: hidden;
    backdrop-filter: brightness(0.8) blur(32px);
    margin-top: 16px;
  }
`
