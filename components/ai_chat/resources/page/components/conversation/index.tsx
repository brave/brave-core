// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useConversation } from '../../state/conversation_context'

export default function Conversation(props: {}) {
  const context = useConversation()

  return (
    <div>
      <ul>
        {context.conversationHistory.map((turn, index) => (
          <li key={index}>
            {turn.characterType}: {turn.text}
          </li>
        ))}
      </ul>
    </div>
  )
}
