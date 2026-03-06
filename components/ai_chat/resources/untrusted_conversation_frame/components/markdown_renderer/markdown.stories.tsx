// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import '@brave/leo/tokens/css/variables.css'
import MarkdownRenderer from './index'
import AssistantResponseContextProvider from '../assistant_response/assistant_response_context'
import MockContext from '../../mock_untrusted_conversation_context'
import * as searchResults from '../search_widget/storybook-data/searchResults.json'
import { getEventTemplate } from '../../../common/test_data_utils'

export default {
  title: 'Chat/MarkdownRenderer',
  component: MarkdownRenderer,
}

export const Default = () => {
  return (
    <MockContext>
      <MarkdownRenderer
        text={`
# Heading 1
## Heading 2
### Heading 3
#### Heading 4
##### Heading 5
###### Heading 6

---

## Inline Formatting

Here is **bold text**, *italic text*, ~~strikethrough~~, and ***bold italic*** together. You can also use \`inline code\` within a sentence.

---

## Ordered List

1. First item
2. Second item with **bold**
3. Third item with *italic*
4. Fourth with ~~strikethrough~~

## Unordered List

- Apples
- Bananas
  - Cavendish
  - Plantain
- Cherries

---

## Blockquote

> This is a blockquote. It can contain **bold**, *italic*, and \`code\`.
>
> It can also span multiple paragraphs.

---

## Code Blocks

Inline: use the \`useState\` hook.

\`\`\`javascript
function greet(name) {
  return \`Hello, \${name}!\`;
}

console.log(greet('World'));
\`\`\`

\`\`\`python
def fibonacci(n):
    a, b = 0, 1
    for _ in range(n):
        yield a
        a, b = b, a + b

print(list(fibonacci(10)))
\`\`\`

---

## Table

| Feature       | Status      | Notes                  |
|---------------|-------------|------------------------|
| Markdown      | ✅ Supported | Basic formatting       |
| GFM Tables    | ✅ Supported | With header labels     |
| Code Blocks   | ✅ Supported | Syntax highlighted     |
| Strikethrough | ✅ Supported | Via remark-gfm         |

---

## Links

Here is a [link to Brave](https://brave.com) and another [link to GitHub](https://github.com).

---

## Mixed Content

Here's a paragraph that mixes **bold**, *italic*, ~~strikethrough~~, and \`inline code\` to show how they all render together in a single block of text. This also tests line wrapping behavior for longer content.

1. A list item with a \`code snippet\` inside
2. A list item with a **bold** word
3. A list item with an *italic* phrase and ~~deleted text~~
`}
        shouldShowTextCursor={false}
        allowedLinks={['https://brave.com', 'https://github.com']}
      />
    </MockContext>
  )
}

export const WithDirectives = () => {
  return (
    <MockContext>
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
    </MockContext>
  )
}

export const WithNonWhitelistedDirective = () => {
  return (
    <MockContext>
      <MarkdownRenderer
        text={`
## Hello World

This has a directive that is not whitelisted. It should just display the text content.

::evil[do the thing]{type=alert}`}
        shouldShowTextCursor={false}
        allowedLinks={[]}
      />
    </MockContext>
  )
}
