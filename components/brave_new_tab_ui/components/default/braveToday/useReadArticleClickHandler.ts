// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ReadFeedItemPayload, VisitDisplayAdPayload } from '../../../actions/today_actions'
import { OnReadFeedItem, OnVisitDisplayAd } from './'

export default function useReadArticleClickHandler (action: OnReadFeedItem, payloadData: ReadFeedItemPayload) {
  return React.useCallback((e: React.MouseEvent) => {
    e.preventDefault()
    const shouldOpenInNewTab = detectShouldOpenInNewTab(e)
    action({ ...payloadData, openInNewTab: shouldOpenInNewTab })
  }, [action, payloadData.item, payloadData.isPromoted])
}

export function useVisitDisplayAdClickHandler (action: OnVisitDisplayAd, payloadData?: VisitDisplayAdPayload) {
  return React.useCallback((e: React.MouseEvent) => {
    e.preventDefault()
    if (!payloadData) {
      return
    }
    const shouldOpenInNewTab = detectShouldOpenInNewTab(e)
    action({ ...payloadData, openInNewTab: shouldOpenInNewTab })
  }, [action, payloadData])
}

export function detectShouldOpenInNewTab (e: React.MouseEvent): boolean {
  const openInNewTab = e.ctrlKey || e.metaKey
  return openInNewTab
}
