/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'
import OptIn from '../../../../../components/brave_news/browser/resources/OptIn'
import NewsPage from '../../../../../components/brave_news/browser/resources/Page'
import CustomizeModal from '../../../../../components/brave_news/browser/resources/customize/Modal'

import { style } from './news_feed.style'

export function NewsFeed() {
  const braveNews = useBraveNews()
  return <>
    {
      braveNews.isShowOnNTPPrefEnabled &&
        <div data-css-scope={style.scope}>
          {braveNews.isOptInPrefEnabled ? <NewsPage /> : <OptIn />}
        </div>
    }
    {braveNews.customizePage && <CustomizeModal />}
  </>
}
