// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.]

import * as React from 'react'
import Editable from './editable'
import { makeEdit, Content } from './editable_content'
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
    const ref = React.useRef<HTMLDivElement>(null)

    return (
      <div
        onKeyDown={(e) => {
          if (e.key === 'Tab') {
            e.preventDefault()

            makeEdit(ref.current!)
              .selectRangeToTriggerChar('/')
              .replaceSelectedRange({
                type: 'skill',
                id: '1',
                text: document.getSelection()?.toString() ?? '',
              })
          }
        }}
      >
        <span>Type /xxxx&lt;tab&gt; to insert a tag</span>
        <hr />
        <Editable
          ref={ref}
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
