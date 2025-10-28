// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.]

import * as React from 'react'
import Editable from './editable'
import {
  replaceRange,
  getRangeToTriggerChar,
  Content,
} from './editable_content'
import { Meta } from '@storybook/react'

export const _Editable = {
  render: () => {
    const [content, setContent] = React.useState<Content>([
      'Hello',
      {
        type: 'skill',
        id: '1',
        text: '/do-the-thing',
      },
      'World',
    ])

    return (
      <div
        onKeyDown={(e) => {
          if (e.key === 'Tab') {
            e.preventDefault()
            const range = getRangeToTriggerChar('/')
            if (!range) return
            replaceRange(range, {
              type: 'skill',
              id: '1',
              text: range.extractContents().textContent ?? '',
            })
          }
        }}
      >
        <span>Type /xxxx&lt;tab&gt; to insert a tag</span>
        <hr />
        <Editable
          placeholder='Type something...'
          content={content}
          onContentChange={setContent}
        />
      </div>
    )
  },
}

export default {
  title: 'Chat/Chat',
  component: Editable,
} as Meta<typeof Editable>
