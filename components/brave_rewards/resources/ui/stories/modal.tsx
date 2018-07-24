/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { withState } from '@dump247/storybook-state'
import { storiesOf, addDecorator } from '@storybook/react'
import { withKnobs, text } from '@storybook/addon-knobs'
import { BetterPageVisualizer } from '../../storyUtil'

// Components
import { TabsType } from '../../../src/features/rewards/modalBackupRestore'
import { DetailRow } from '../../../src/features/rewards/contributeTable'
import { ModalContribute, ModalBackupRestore } from '../../../src/features/rewards'

const bart = require('../../assets/img/bartBaker.jpeg')
const ddgo = require('../../assets/img/ddgo.jpg')
const wiki = require('../../assets/img/wiki.jpg')
const buzz = require('../../assets/img/buzz.jpg')
const guardian = require('../../assets/img/guardian.jpg')

addDecorator(withKnobs)

// Globally adapt the story visualizer for this story
addDecorator(BetterPageVisualizer)

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Feature Components/Rewards/Modal', module)
  .add('Backup/Restore', withState({ tabId: 'backup' }, (store) => {
    const onTabChange = (tabId: string) => {
      store.set({ tabId })
    }
    return (
      <div style={{ maxWidth: '900px', background: '#fff', padding: '30px' }}>
        <ModalBackupRestore
          activeTabId={store.state.tabId as TabsType}
          recoveryKey={'crouch  hint  glow  recall  round  angry  weasel  luggage save  hood  census  near  still   power  vague  balcony camp  law  now  certain  wagon  affair  butter  choice '}
          error={text('Error', '')}
          onTabChange={onTabChange}
          onClose={doNothing}
          onCopy={doNothing}
          onPrint={doNothing}
          onSaveFile={doNothing}
          onRestore={doNothing}
          onImport={doNothing}
        />
      </div>
    )
  }))
  .add('Contribute', () => {
    const rows: DetailRow[] = [
      {
        profile: {
          name: 'Bart Baker',
          verified: true,
          provider: 'youtube',
          src: bart
        },
        attention: 40,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: ddgo
        },
        attention: 20,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: buzz
        },
        attention: 10,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: guardian
        },
        attention: 5,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: wiki
        },
        attention: 4,
        onRemove: doNothing
      }
    ]

    return (
      <ModalContribute
        rows={rows}
        onClose={doNothing}
      />
    )
  })
