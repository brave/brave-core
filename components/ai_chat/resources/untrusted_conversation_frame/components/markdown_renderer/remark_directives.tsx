// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { CONTINUE, SKIP, visit } from 'unist-util-visit'
import type { Node } from 'unist'

export const ALLOWED_DIRECTIVES = ['search'] as const

export function remarkDirectives() {
  return (tree: Node) => {
    // Note: Currently, we only support leaf directives, but there's no
    // technical limitation its just the only thing we need so far.
    visit(tree, ['leafDirective'], (node) => {
      const name = (node as any).name

      // Only allow whitelisted directives.
      if (!ALLOWED_DIRECTIVES.includes(name)) {
        return SKIP
      }

      node.data = {
        hName: name,
        hProperties: (node as any).attributes,
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
  // Note: I've enabled this directive but it doesn't render anything yet.
  search: function () {
    return null
  },
}
