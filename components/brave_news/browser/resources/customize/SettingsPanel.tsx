/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  color,
  effect,
  radius,
  spacing,
} from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import styled from 'styled-components'

const Panel = styled.div`
  h4 {
    padding-bottom: ${spacing.xl};
    color: ${color.text.primary};
  }
`

const Content = styled.div`
  background: ${color.container.background};
  box-shadow: ${effect.elevation['01']};
  border-radius: ${radius.xl};
`

interface Props {
  title: string
  children: React.ReactNode
}

export function SettingsPanel(props: Props) {
  return (
    <Panel>
      <h4>{props.title}</h4>
      <Content>{props.children}</Content>
    </Panel>
  )
}
