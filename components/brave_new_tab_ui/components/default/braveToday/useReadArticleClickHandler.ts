// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ReadFeedItemPayload } from '../../../actions/today_actions'
import { OnReadFeedItem } from './'

export default function useReadArticleClickHandler (action: OnReadFeedItem, payloadData: ReadFeedItemPayload) {
  return React.useCallback((e: React.MouseEvent) => {
    const openInNewTab = e.ctrlKey || e.metaKey
    action({ ...payloadData, openInNewTab })
    e.preventDefault()
  }, [action, payloadData.item, payloadData.isPromoted])
}
