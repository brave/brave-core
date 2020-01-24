/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button } from 'brave-ui'

// Component-specific components
import {
  DisabledContent,
  Main,
  SyncCard,
  SyncCardContent,
  Title,
  DisabledContentButtonGrid,
  TableGrid,
  Paragraph
} from '../components'

// Modals
import EnterSyncCode from './modals/enterSyncCode'

// Images
import { SyncStartIcon } from '../components/images'

// Utils
import { getLocale } from './page/fakeLocale'
import data from './page/fakeData'

interface State {
  newToSync: boolean
  existingSyncCode: boolean
}

export default class SyncDisabledContent extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      newToSync: false,
      existingSyncCode: false
    }
  }

  get mainDeviceName () {
    return data.device1.name
  }

  onClickNewSyncChainButton = () => {
    this.setState({ newToSync: !this.state.newToSync })
  }

  onClickEnterSyncChainCodeButton = () => {
    this.setState({ existingSyncCode: !this.state.existingSyncCode })
  }

  render () {
    const { existingSyncCode } = this.state
    return (
      <DisabledContent>
        <Main>
          {
            existingSyncCode
              ? <EnterSyncCode onClose={this.onClickEnterSyncChainCodeButton} />
              : null
          }
          <SyncCard>
            <SyncCardContent>
              <TableGrid isDeviceTable={false}>
                <SyncStartIcon />
                <div>
                  <Title level={2}>{getLocale('syncTitle')}</Title>
                  <Paragraph>{getLocale('syncDescription')}</Paragraph>
                  <DisabledContentButtonGrid>
                    <Button
                      level='primary'
                      type='accent'
                      onClick={this.onClickNewSyncChainButton}
                      text={getLocale('startSyncChain')}
                    />
                    <Button
                      level='secondary'
                      type='accent'
                      onClick={this.onClickEnterSyncChainCodeButton}
                      text={getLocale('enterSyncChainCode')}
                    />
                  </DisabledContentButtonGrid>
                </div>
              </TableGrid>
            </SyncCardContent>
          </SyncCard>
        </Main>
      </DisabledContent>
    )
  }
}
