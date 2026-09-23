// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import { getToolUseEvent } from '../../../common/test_data_utils'
import { createTextContentBlock } from '../../../common/content_block'

export const toolUseCompleteAssistantDetailStorage = getToolUseEvent({
  id: 'abc123d',
  toolName: Mojom.ASSISTANT_DETAIL_STORAGE_TOOL_NAME,
  argumentsJson: JSON.stringify({
    information:
      'This is some data that the LLM wants to store for later before other tool use responses get removed from context because they are too large',
  }),
  output: [createTextContentBlock('Stored, refer to input for data')],
})
