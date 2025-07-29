/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useNewTabState } from './new_tab_context'

const AIChatProviders = React.lazy(() => import('./ai_chat_providers'))

export function AIChatProvider(props: { children: React.ReactNode }) {
  const aiChatInputEnabled = useNewTabState((s) => s.aiChatInputEnabled)
  if (!aiChatInputEnabled) {
    return <>{props.children}</>
  }
  return (
    <AIChatProviders>{props.children}</AIChatProviders>
  )
}
