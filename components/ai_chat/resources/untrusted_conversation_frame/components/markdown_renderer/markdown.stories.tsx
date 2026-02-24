// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import MarkdownRenderer from './index'
import AssistantResponseContextProvider from '../assistant_response/assistant_response_context'
import * as searchResults from '../search_widget/storybook-data/searchResults.json'
import { getEventTemplate } from '../../../common/test_data_utils'

export default {
  title: 'Chat/MarkdownRenderer',
  component: MarkdownRenderer,
}

export const Default = () => {
  return (
    <MarkdownRenderer
      text={`
## Some Markdown

A list
1. Item 1 ~~strike~~
2. Item 2 **bold**
3. Item 3 *italic*`}
      shouldShowTextCursor={false}
      allowedLinks={[]}
    />
  )
}

export const WithDirectives = () => {
  return (
    <AssistantResponseContextProvider
      events={[
        {
          ...getEventTemplate(),
          inlineSearchEvent: {
            query: 'Approach shoes',
            resultsJson: JSON.stringify(Array.from(searchResults)),
          },
        },
      ]}
    >
      <MarkdownRenderer
        text={`
## Hello World

This is some text about a product followed by a directive.

::search[Approach shoes]{type=web}`}
        shouldShowTextCursor={false}
        allowedLinks={[]}
      />
    </AssistantResponseContextProvider>
  )
}

export const WithNonWhitelistedDirective = () => {
  return (
    <MarkdownRenderer
      text={`
## Hello World

This has a directive that is not whitelisted. It should just display the text content.

::evil[do the thing]{type=alert}`}
      shouldShowTextCursor={false}
      allowedLinks={[]}
    />
  )
}
