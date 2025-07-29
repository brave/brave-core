/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useSearchState } from '../../context/search_context'
import { usePersistedState } from '$web-common/usePersistedState'
import { ChatInput } from './chat_input'
import { SearchInput } from './search_input'
import { QueryModeToggle, QueryMode } from './query_mode_toggle'

import { style } from './query_box.style'

interface Props {
  showSearchSettings: () => void
}

export function QueryBox(props: Props) {
  const searchFeatureEnabled = useSearchState((s) => s.searchFeatureEnabled)
  const showSearchBox = useSearchState((s) => s.showSearchBox)

  const [queryMode, setQueryMode] = usePersistedState<QueryMode>({
    key: 'ntp-query-input-mode',
    parse: (value) => value === 'chat' ? value : 'search',
    stringify: (state) => state
  })

  const renderToggle = React.useMemo(() => {
    return () => (
      <QueryModeToggle queryMode={queryMode} onChange={setQueryMode}/>
    )
  }, [queryMode])

  if (!searchFeatureEnabled || !showSearchBox) {
    return null
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='query-container'>
        <div className='input-container'>
          {
            queryMode === 'search' ?
              <SearchInput
                showSearchSettings={props.showSearchSettings}
                renderInputControls={renderToggle}
              /> :
              <ChatInput
                renderInputControls={renderToggle}
              />
          }
        </div>
      </div>
    </div>
  )
}

export default QueryBox
