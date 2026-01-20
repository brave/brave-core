/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AIChatProvider } from '../../context/ai_chat_context'
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
  const showChatInput = useSearchState((s) => s.showChatInput)

  const [queryMode, setQueryMode] = usePersistedState<QueryMode>({
    key: 'ntp-query-input-mode',
    parse: (value) => (value === 'chat' ? value : 'search'),
    stringify: (state) => state,
  })

  const canToggle = showSearchBox && showChatInput

  const shouldShowSearch = () => {
    if (canToggle) {
      return queryMode === 'search'
    }
    return !showChatInput
  }

  const renderToggle = React.useMemo(() => {
    if (!canToggle) {
      return undefined
    }
    return () => (
      <QueryModeToggle
        queryMode={queryMode}
        onChange={setQueryMode}
      />
    )
  }, [queryMode, showSearchBox, showChatInput])

  if (!searchFeatureEnabled || (!showSearchBox && !showChatInput)) {
    return null
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='query-container'>
        <div className='input-container'>
          <AIChatProvider>
            {shouldShowSearch() ? (
              <SearchInput
                showSearchSettings={props.showSearchSettings}
                renderInputToggle={renderToggle}
              />
            ) : (
              <ChatInput renderInputToggle={renderToggle} />
            )}
          </AIChatProvider>
        </div>
      </div>
    </div>
  )
}

export default QueryBox
