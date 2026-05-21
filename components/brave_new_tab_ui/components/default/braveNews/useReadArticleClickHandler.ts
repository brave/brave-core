// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ReadFeedItemPayload } from '../../../actions/today_actions'
import { OnReadFeedItem } from './'

export default function useReadArticleClickHandler (action: OnReadFeedItem, payloadData: ReadFeedItemPayload) {
  return React.useCallback((e: React.MouseEvent) => {
    e.preventDefault()
    const shouldOpenInNewTab = detectShouldOpenInNewTab(e)
    action({ ...payloadData, openInNewTab: shouldOpenInNewTab })
  }, [action, payloadData.item])
}

export function detectShouldOpenInNewTab (e: React.MouseEvent): boolean {
  const openInNewTab = e.ctrlKey || e.metaKey
  return openInNewTab
}
