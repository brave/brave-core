// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { boolean, select, text } from '@storybook/addon-knobs'
import { withState } from '@dump247/storybook-state'
import {
  List
} from '../components'
import {
  BoxMobile,
  MainToggleMobile
} from '../components/mobile'

export default {
  title: 'Rewards/Other/Mobile',
  parameters: {
    layout: 'centered'
  }
}

export const Box = withState({ checked: true, toggle: true }, (store) => {
  const onToggle = () => {
    store.set({ checked: !store.state.checked })
  }

  return (
    <div style={{ width: '100%' }}>
      <BoxMobile
        title={text('Title', 'Brave Auto-Contribute')}
        toggle={boolean('Show toggle', store.state.toggle)}
        checked={boolean('Toggle checked', store.state.checked)}
        type={select<any>('Type', { contribute: 'contribute', donation: 'donation', ads: 'ads' }, 'contribute')}
        description={
          text('Description', 'A simple way to support content creators. Set monthly allowance and browse normally. Your favorite sites (content sites only) receive your contributions automatically. You can exclude sites from funding right from the list below.')
        }
        toggleAction={onToggle}
        settingsChild={<div>Settings content</div>}
      >
        <div style={{ padding: '0px 25px' }}>
          <p>Detail Content</p>
        </div>
      </BoxMobile>
    </div>
  )
})
export const mainToggle = () => {
  let items = []
  for (let i = 0; i < 25; i++) {
    items.push(i)
  }
  return (
    <div style={{ width: '100%' }}>
      <MainToggleMobile/>
      {items.map(i =>
        <div key={i}>
          <List title={`Item No: ${i}`} />
        </div>
      )}
    </div>
  )
}
mainToggle.storyName = 'Main Toggle'
