/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useSearchState } from '../../context/search_context'
import { useNewTabState } from '../../context/new_tab_context'
import { SearchBox } from '../search/search_box'
import { Popover } from '../common/popover'
import { ChatInput } from './chat_input'
import { SearchInput } from './search_input'
import { parseQueryMode, QueryMode, QueryModeToggle } from './query_mode_toggle'
import classNames from '$web-common/classnames'

import { style } from './query_box.style'

interface Props {
  showSearchSettings: () => void
}

export function QueryBox(props: Props) {
  const searchFeatureEnabled = useSearchState((s) => s.searchFeatureEnabled)
  const chatFeatureEnabled = useNewTabState((s) => s.aiChatFeatureEnabled)
  const showSearchBox = useSearchState((s) => s.showSearchBox)
  const [expanded, setExpanded] = React.useState(false)
  const [queryMode, setQueryMode] = React.useState(loadQueryModeState)

  React.useEffect(() => {
    document.body.classList.toggle('search-box-expanded', expanded)
  }, [expanded])

  React.useEffect(() => { saveQueryModeState(queryMode) }, [queryMode])

  if (!searchFeatureEnabled || !showSearchBox) {
    return null
  }

  if (!chatFeatureEnabled) {
    return <SearchBox showSearchSettings={props.showSearchSettings} />
  }

  const renderToggle = React.useMemo(() => {
    return () => (
      <QueryModeToggle queryMode={queryMode} onChange={setQueryMode}/>
    )
  }, [queryMode])

  return (
    <div data-css-scope={style.scope} className={classNames({ expanded })}>
      <Popover
        isOpen={expanded}
        className='query-container'
        onClose={() => setExpanded(false)}
      >
        <div className='input-container'>
          {
            queryMode === 'search' ?
              <SearchInput
                showSearchSettings={props.showSearchSettings}
                onActivated={() => setExpanded(true)}
                renderInputControls={renderToggle}
              /> :
              <ChatInput
                renderInputControls={renderToggle}
              />
          }
        </div>
      </Popover>
    </div>
  )
}

const queryModeStorageKey = 'ntp-query-mode'

function loadQueryModeState(): QueryMode {
  const value = localStorage.getItem(queryModeStorageKey)
  return parseQueryMode(value || '') || 'search'
}

function saveQueryModeState(queryMode: QueryMode) {
  localStorage.setItem(queryModeStorageKey, queryMode)
}
