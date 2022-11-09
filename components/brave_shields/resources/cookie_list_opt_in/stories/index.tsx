// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

import MainPanel from '../components/main-panel'
import './locale'

const BubbleStyle = styled.div`
  box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  border-radius: 12px;
  background: #fff;
  width: 388px;
  overflow: hidden;
`

export default {
  title: 'Shields/Cookie Consent Blocker',
  decorators: [
    (Story: any) => {
      return (
        <BubbleStyle>
          <Story />
        </BubbleStyle>
      )
    }
  ]
}

export const _Main = () => {
  return (
    <MainPanel
      onEnable={() => { console.log('enabled') }}
      onDismiss={() => { console.log('dismissed') }}
      onDecline={() => { console.log('declined') }}
      onAnimationComplete={() => { console.log('animation complete') }}
    />
  )
}
