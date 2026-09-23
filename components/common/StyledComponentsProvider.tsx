// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { StyleSheetManager } from 'styled-components'
import isPropValid from '@emotion/is-prop-valid'

// styled-components v6 forwards every prop to the underlying host element by
// default. In v5 it filtered props through @emotion/is-prop-valid so custom
// style-only props (e.g. `selected`, `otherWidgetsHidden`) never reached the
// DOM. This restores that v5 behavior globally so those props don't leak onto
// DOM nodes as invalid attributes.
// https://styled-components.com/docs/faqs#shouldforwardprop-is-no-longer-provided-by-default
export function shouldForwardProp(propName: string, target: unknown) {
  // Only filter props for host elements (string targets like 'div'). Custom
  // React components are responsible for their own props and should receive
  // everything.
  return typeof target === 'string' ? isPropValid(propName) : true
}

// Wrap an app's root in this provider so all styled-components below it filter
// DOM props the same way they did under styled-components v5.
export default function StyledComponentsProvider(
  props: React.PropsWithChildren
) {
  return (
    <StyleSheetManager shouldForwardProp={shouldForwardProp}>
      {props.children}
    </StyleSheetManager>
  )
}
