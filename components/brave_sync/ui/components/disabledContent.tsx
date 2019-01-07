/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button } from 'brave-ui'

// Component-specific components
import {
  Main,
  Title,
  DisabledContentButtonGrid,
  TableGrid,
  Paragraph,
  SyncCard,
  DisabledContent
} from 'brave-ui/features/sync'

import { SyncStartIcon } from 'brave-ui/features/sync/images'

// Modals
import DeviceTypeModal from './modals/deviceType'
import EnterSyncCodeModal from './modals/enterSyncCode'

// Utils
import { getLocale } from '../../../common/locale'

interface Props {
  syncData: Sync.State
  actions: any
}

interface State {
  newToSync: boolean
  existingSyncCode: boolean
}

export default class SyncDisabledContent extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      newToSync: false,
      existingSyncCode: false
    }
  }

  onClickNewSyncChainButton = () => {
    this.setState({ newToSync: !this.state.newToSync })
  }

  onClickEnterSyncChainCodeButton = () => {
    this.setState({ existingSyncCode: !this.state.existingSyncCode })
  }

  render () {
    const { actions, syncData } = this.props
    const { newToSync, existingSyncCode } = this.state

    if (!syncData) {
      return null
    }

    return (
      <DisabledContent>
        <Main>
          {
            newToSync
              ? <DeviceTypeModal syncData={syncData} actions={actions} onClose={this.onClickNewSyncChainButton} />
              : null
          }
          {
            existingSyncCode
              ? <EnterSyncCodeModal syncData={syncData} actions={actions} onClose={this.onClickEnterSyncChainCodeButton} />
              : null
          }
          <SyncCard>
            <TableGrid>
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
          </SyncCard>
        </Main>
      </DisabledContent>
    )
  }
}
