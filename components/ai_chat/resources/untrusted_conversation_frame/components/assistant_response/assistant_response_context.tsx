// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as Mojom from '../../../common/mojom'

export const AssistantResponseContext = React.createContext<{
  events: Mojom.ConversationEntryEvent[]
}>({
  events: [],
})

export default function AssistantResponseContextProvider(props: {
  events: Mojom.ConversationEntryEvent[]
  children: React.ReactNode
}) {
  const context = React.useMemo(() => {
    return {
      events: props.events,
    }
  }, [props.events])
  return (
    <AssistantResponseContext.Provider value={context}>
      {props.children}
    </AssistantResponseContext.Provider>
  )
}

export const useAssistantEvents = () => {
  const { events } = React.useContext(AssistantResponseContext)
  return events
}
