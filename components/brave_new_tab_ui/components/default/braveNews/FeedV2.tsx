// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { api } from '../../../../brave_news/browser/resources/context'
import usePromise from '$web-common/usePromise'
import CardLoading from './cards/cardLoading'

const FeedItems = React.lazy(() => import('../../../../brave_news/browser/resources/Feed'))

export const FeedV2 = () => {
  const { result: feed } = usePromise(() => api.getFeedV2(), [])

  return <React.Suspense fallback={<CardLoading />}>
    <FeedItems feed={feed?.feed} />
  </React.Suspense>
}
