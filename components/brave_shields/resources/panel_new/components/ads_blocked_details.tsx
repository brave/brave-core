/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useShieldsApi } from '../api/shields_api_context'
import { getString } from './strings'
import { DetailsHeader } from './details_header'
import { ResourceListView } from './resource_list_view'

import { style } from './ads_blocked_details.style'

interface Props {
  onBack: () => void
}

export function AdsBlockedDetails(props: Props) {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const adsList = siteBlockInfo?.adsList ?? []
  const title = getString('BRAVE_SHIELDS_TRACKERS_ADS_BLOCKED_TITLE')
  return (
    <main data-css-scope={style.scope}>
      <DetailsHeader
        title={`${title} (${adsList.length})`}
        onBack={props.onBack}
      />
      <div className='resource-list scrollable'>
        <ResourceListView urls={adsList.map((url) => url.url)} />
      </div>
    </main>
  )
}
