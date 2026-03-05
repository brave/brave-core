/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useSearchState } from '../../context/search_context'
import { usePersistedState } from '$web-common/usePersistedState'
import { SearchInput } from './search_input'
import { QueryModeToggle, QueryMode } from './query_mode_toggle'
import { MaybeAIChatContext } from '../../context/maybe_ai_chat_context'

import { style } from './query_box.style'

// ChatInput is lazy loaded to reduce JS bytes when the user has disabled the
// chat input on the NTP.
const ChatInput = React.lazy(() => import('./chat_input'))

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
          <MaybeAIChatContext>
            {shouldShowSearch() ? (
              <SearchInput
                showSearchSettings={props.showSearchSettings}
                renderInputToggle={renderToggle}
              />
            ) : (
              <React.Suspense>
                <ChatInput renderInputToggle={renderToggle} />
              </React.Suspense>
            )}
          </MaybeAIChatContext>
        </div>
      </div>
    </div>
  )
}
