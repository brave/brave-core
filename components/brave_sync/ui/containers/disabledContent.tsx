/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button } from 'brave-ui'
import { LoaderIcon } from 'brave-ui/components/icons'

// Component-specific components
import {
  Main,
  Title,
  DisabledContentButtonGrid,
  TableGrid,
  Paragraph,
  SyncCard,
  SyncCardContent,
  DisabledContent
} from '../components'

import { SyncStartIcon } from '../components/images'

// Modals
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
    // once the screen is rendered, create a sync chain.
    // this allow us to request the qr code and sync words immediately
    const { thisDeviceName } = this.props.syncData
    if (thisDeviceName === '') {
      this.props.actions.onSetupNewToSync('')
    }
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
            existingSyncCode
              ? <EnterSyncCodeModal syncData={syncData} actions={actions} onClose={this.onClickEnterSyncChainCodeButton} />
              : null
          }
          <SyncCard>
            <SyncCardContent>
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
                      icon={{
                        position: 'before',
                        image: newToSync === true
                          ? <LoaderIcon />
                          : null
                      }}
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
