// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

function fixCssNumericProp (prop: number | string | undefined) {
  return prop && typeof prop !== 'string'
    ? isNaN(prop)
      ? undefined
      : prop
    : undefined
}

export const StyledDiv = styled.div.attrs((a) => ({
  style: {
    ...a.style,
    height: 'auto'
  }
}))`
  top: ${(p) => fixCssNumericProp(p.style?.top) || '0px'};
`
