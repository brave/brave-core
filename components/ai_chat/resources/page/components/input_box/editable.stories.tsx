// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.]

import * as React from 'react'
import Editable, { Content } from './editable'
import { replaceRange, getRangeToTriggerChar } from './ranges'
import { Meta } from '@storybook/react'

export const _Editable = {
  render: () => {
    const [content, setContent] = React.useState<Content>(['Hello', {
        type: 'associated-content',
        id: '1',
        text: 'My Tab'
      }, 'World'])

    return <div onKeyDown={e => {
      if (e.key === 'Tab') {
        e.preventDefault()
        const range = getRangeToTriggerChar('@')
        if (!range) return
        replaceRange(range, {
          type: 'associated-content',
          id: '1',
          text: range.extractContents().textContent?.substring(1) ?? ''
        })
      }
    }
    } >
      <Editable placeholder='Type something...' content={content} onContentChange={setContent} />
    </div >
  },
}

export default {
  title: 'Chat/Chat',
  component: Editable,
} as Meta<typeof Editable>
