// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../common/mojom'

interface Props {
  // Entries that make up the task loop
  assistantEntries: Mojom.ConversationTurn[]
}

export default function AssistantTask(props: Props) {
  return <h1>Task stub</h1>
}
