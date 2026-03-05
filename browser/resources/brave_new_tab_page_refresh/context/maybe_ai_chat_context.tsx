/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useNewTabState } from './new_tab_context'
import { useSearchState } from './search_context'

// <if expr="is_storybook">
import { MockContext } from '../../../../components/ai_chat/resources/page/state/mock_context'
// <else>
const AIChatContext = React.lazy(() => import('./ai_chat_context'))
// </if>

export function MaybeAIChatContext(props: React.PropsWithChildren) {
  const aiChatInputEnabled = useNewTabState((s) => s.aiChatInputEnabled)
  const showChatInput = useSearchState((s) => s.showChatInput)

  if (!aiChatInputEnabled || !showChatInput) {
    return props.children
  }

  // <if expr="is_storybook">
  return <MockContext>{props.children}</MockContext>
  // <else>
  return (
    <React.Suspense>
      <AIChatContext>{props.children}</AIChatContext>
    </React.Suspense>
  )
  // </if>
}
