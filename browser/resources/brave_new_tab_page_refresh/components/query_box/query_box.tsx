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

// <if expr="is_storybook">
import { MockContext as MockAIChatContext } from '../../../../../components/ai_chat/resources/page/state/mock_context'
// <else>
import AIChatContext from '../../context/ai_chat_context'
// </if>

interface Props {
  showSearchSettings: () => void
}

/**
 * Don't render AIChatContext, creating mojom bindings and receiving state
 * updates unneccessarily if we're not showing chat input. But do render it
 * even when toggled to search when the AI Chat input is toggleable, so that
 * we preserve any input text.
 */
function MaybeAIChatContext(
  props: React.PropsWithChildren<{ shouldRenderContext: boolean }>,
) {
  if (!props.shouldRenderContext) {
    return props.children
  }

  // Since this is both lazy-loaded and conditionally-loaded, it isn't as easy
  // as other stores to have the base app provide the correct API classes wired
  // up to the contexts. But it's easy to import them directly here, and still
  // ensure the mock contexts don't end up in the regular build.
  let ContextProvider: (props: React.PropsWithChildren) => JSX.Element
  // <if expr="is_storybook">
  ContextProvider = MockAIChatContext
  // <else>
  ContextProvider = AIChatContext
  // </if>

  return <ContextProvider>{props.children}</ContextProvider>
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
          <MaybeAIChatContext shouldRenderContext={showChatInput}>
            {shouldShowSearch() ? (
              <SearchInput
                showSearchSettings={props.showSearchSettings}
                renderInputToggle={renderToggle}
              />
            ) : (
              <ChatInput renderInputToggle={renderToggle} />
            )}
          </MaybeAIChatContext>
        </div>
      </div>
    </div>
  )
}

export default QueryBox
