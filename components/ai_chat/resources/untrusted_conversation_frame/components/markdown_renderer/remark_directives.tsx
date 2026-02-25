// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { CONTINUE, SKIP, visit } from 'unist-util-visit'
import type { Data, Node } from 'unist'
import SearchWidget from '../search_widget/search_widget'
import * as React from 'react'
import { useAssistantEvents } from '../assistant_response/assistant_response_context'

export const ALLOWED_DIRECTIVES = ['search'] as const
type NodeType = Node<Data> & {
  name: string
  attributes: Record<string, string>
}

export function remarkDirectives() {
  return (tree: NodeType) => {
    // Note: Currently, we only support leaf directives, but there's no
    // technical limitation its just the only thing we need so far.
    visit(tree, ['leafDirective'], (node) => {
      const name = node.name

      // Only allow whitelisted directives.
      if (!(ALLOWED_DIRECTIVES as readonly string[]).includes(name)) {
        return SKIP
      }

      node.data = {
        hName: name,
        hProperties: node.attributes,
        ...node.data,
      }

      return CONTINUE
    })
  }
}

// Component types for supported directives.
export const directiveComponents: Record<
  (typeof ALLOWED_DIRECTIVES)[number],
  (props: any) => string | JSX.Element | null
> = {
  search: function (props: any) {
    const events = useAssistantEvents()
    const inlineSearchEvent = React.useMemo(
      () =>
        events.find(
          (event) => event.inlineSearchEvent?.query === props.children,
        )?.inlineSearchEvent,
      [events, props.children],
    )
    const results = React.useMemo(() => {
      if (!inlineSearchEvent) return []
      try {
        return JSON.parse(inlineSearchEvent.resultsJson)
      } catch {
        return []
      }
    }, [inlineSearchEvent])

    return (
      <SearchWidget
        query={props.children}
        type={props.type}
        results={results}
      />
    )
  },
}
