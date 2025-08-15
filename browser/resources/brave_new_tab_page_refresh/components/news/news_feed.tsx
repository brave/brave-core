/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'
import { useNewTabState } from '../../context/new_tab_context'

import { style } from './news_feed.style'

const OptIn = React.lazy(() => import(
  '../../../../../components/brave_news/browser/resources/OptIn'))
const NewsPage = React.lazy(() => import(
  '../../../../../components/brave_news/browser/resources/Page'))
const CustomizeModal = React.lazy(() => import(
  '../../../../../components/brave_news/browser/resources/customize/Modal'))

export function NewsFeed() {
  const braveNews = useBraveNews()
  const newsFeatureEnabled = useNewTabState((s) => s.newsFeatureEnabled)
  const [shouldRenderNews, setShouldRenderNews] = React.useState(false)

  React.useEffect(() => {
    setTimeout(() => setShouldRenderNews(true), 1000)
  }, [])

  if (!newsFeatureEnabled) {
    return null
  }

  return <>
    {
      braveNews.isShowOnNTPPrefEnabled &&
        <div data-css-scope={style.scope}>
          {shouldRenderNews &&
            (braveNews.isOptInPrefEnabled ? <NewsPage /> : <OptIn />)
          }
        </div>
    }
    {braveNews.customizePage && <CustomizeModal />}
  </>
}
