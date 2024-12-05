// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ConversationEntries from '../../../untrusted_conversation_frame/components/conversation_entries'
import { ConversationEntriesProps } from '../../state/ai_chat_context'


// The real WebUI has an iframe with the ConversationEntries component but
// in storybook it's easier to just include it directly
export default function StorybookConversationEntries (props: ConversationEntriesProps) {
  React.useEffect(() => {
    props.onLoad()
  }, [])

  return (
    <ConversationEntries
      onLastElementHeightChange={props.onGeneratedConversationEntryHeightChanged}
    />
  )
}
