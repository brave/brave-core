/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useShieldsApi } from '../api/shields_api_context'
import { getString } from './strings'
import { DetailsHeader } from './details_header'
import { ResourceListView } from './resource_list_view'

import { style } from './scripts_blocked_details.style'

interface Props {
  onBack: () => void
}

export function ScriptsBlockedDetails(props: Props) {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()

  if (!siteBlockInfo) {
    return null
  }

  const allowedList = siteBlockInfo.allowedJsList.map((item) => item.url)
  const blockedList = siteBlockInfo.blockedJsList.map((item) => item.url)

  return (
    <main data-css-scope={style.scope}>
      <DetailsHeader
        title={getString('BRAVE_SHIELDS_SCRIPT_DETAILS_TITLE')}
        onBack={props.onBack}
      />
      <div className='scrollable'>
        {allowedList.length > 0 && (
          <>
            <div className='list-header allow-header'>
              {getString('BRAVE_SHIELDS_ALLOWED_SCRIPTS_LABEL')} (
              {allowedList.length})
              <button onClick={() => api.blockAllowedScripts([allowedList])}>
                {getString('BRAVE_SHIELDS_BLOCK_SCRIPTS_ALL')}
              </button>
            </div>
            <div className='resource-list'>
              <ResourceListView
                urls={allowedList}
                actionText={getString('BRAVE_SHIELDS_SCRIPT_BLOCK')}
                actionHandler={(url) => api.blockAllowedScripts([[url]])}
              />
            </div>
          </>
        )}
        {blockedList.length > 0 && (
          <>
            <div className='list-header block-header'>
              {getString('BRAVE_SHIELDS_BLOCKED_SCRIPTS_LABEL')} (
              {blockedList.length})
              <button onClick={() => api.allowScriptsOnce([blockedList])}>
                {getString('BRAVE_SHIELDS_ALLOW_SCRIPTS_ALL')}
              </button>
            </div>
            <div className='resource-list'>
              <ResourceListView
                urls={blockedList}
                actionText={getString('BRAVE_SHIELDS_ALLOW_SCRIPT_ONCE')}
                actionHandler={(url) => api.allowScriptsOnce([[url]])}
              />
            </div>
          </>
        )}
      </div>
    </main>
  )
}
