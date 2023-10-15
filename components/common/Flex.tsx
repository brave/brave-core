// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import {
  AlignItemsProperty,
  JustifyContentProperty,
  FlexDirectionProperty,
  GapProperty,
  FlexWrapProperty
} from 'csstype'

interface FlexProps {
  align?: AlignItemsProperty;
  justify?: JustifyContentProperty,
  direction?: FlexDirectionProperty
  gap?: GapProperty<number>,
  wrap?: FlexWrapProperty
}

const Flex = styled('div') <FlexProps>`
  display: flex;
  flex-direction: ${p => p.direction};
  justify-content: ${p => p.justify};
  align-items: ${p => p.align};
  gap: ${p => typeof p.gap === 'number' ? `${p.gap}px` : p.gap};
  flex-wrap: ${p => p.wrap};
`

export default Flex
