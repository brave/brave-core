/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Toggle } from '../../../src/features/shields'
import Button from '../../../src/components/buttonsIndicators/button'
import { CloseCircleOIcon } from '../../../src/components/icons'
import Table, { Cell, Row } from '../../../src/components/dataTables/table'

// Feature-specific components
import {
  EnabledContent,
  Main,
  SyncCard,
  SyncCardContent,
  Title,
  Paragraph,
  SectionBlock,
  TableRowDevice,
  TableRowRemoveButton,
  TableGrid,
  TableButtonGrid,
  TableRowToggleButton
} from '../../../src/features/sync'

// Modals
import RemoveMainDevice from './modals/removeMainDevice'
import RemoveOtherDevice from './modals/removeOtherDevice'
import ViewSyncCodeModal from './modals/viewSyncCode'
import DeviceTypeModal from './modals/deviceType'
import ResetSyncModal from './modals/resetSync'

// Utils
import { getLocale } from './page/fakeLocale'
import data from './page/fakeData'

interface State {
  removeOtherDevice: boolean
  removeMainDevice: boolean
  viewSyncCode: boolean
  addDevice: boolean
  resetSync: boolean
}

export default class SyncEnabledContent extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      removeOtherDevice: false,
      removeMainDevice: false,
      viewSyncCode: false,
      addDevice: false,
      resetSync: false
    }
  }

  get mainDeviceName () {
    return data.device1.name
  }

  get otherDeviceName () {
    return data.device2.name
  }

  get deviceRows (): Row[] {
    return [
      {
        content: [
          {
            content: (
              <TableRowDevice>{data.device1.name} (This Device)</TableRowDevice>
            )
          },
          { content: data.device1.lastActive },
          {
            content: (
              <TableRowRemoveButton
                data-id={''}
                data-name={''}
                onClick={this.onClickRemoveMainDeviceButton}
              >
                <CloseCircleOIcon />
              </TableRowRemoveButton>
            )
          }
        ]
      },
      {
        content: [
          { content: <TableRowDevice>{data.device2.name}</TableRowDevice> },
          { content: data.device2.lastActive },
          {
            content: (
              <TableRowRemoveButton
                data-id={''}
                data-name={''}
                onClick={this.onClickRemoveOtherDeviceButton}
              >
                <CloseCircleOIcon />
              </TableRowRemoveButton>
            )
          }
        ]
      }
    ]
  }

  get deviceHeader (): Cell[] {
    return [
      { content: <TableRowDevice>{getLocale('deviceName')}</TableRowDevice> },
      { content: getLocale('addedOn') },
      { content: '' }
    ]
  }

  get settingsHeader (): Cell[] {
    return [
      { content: <TableRowDevice>{getLocale('settings')}</TableRowDevice> },
      { content: '' }
    ]
  }

  get settingsRows (): Row[] {
    return [
      {
        content: [
          {
            content: getLocale('bookmarks')
          },
          {
            content: (
              <TableRowToggleButton>
                <Toggle id='bookmarks' checked={true} size='large' />
              </TableRowToggleButton>
            )
          }
        ]
      }
    ]
  }

  onClickRemoveOtherDeviceButton = () => {
    this.setState({ removeOtherDevice: !this.state.removeOtherDevice })
  }

  onClickRemoveMainDeviceButton = () => {
    this.setState({ removeMainDevice: !this.state.removeMainDevice })
  }

  onClickViewSyncCodeButton = () => {
    this.setState({ viewSyncCode: !this.state.viewSyncCode })
  }

  onClickAddDeviceButton = () => {
    this.setState({ addDevice: !this.state.addDevice })
  }

  onClickResetSyncButton = () => {
    this.setState({ resetSync: !this.state.resetSync })
  }

  render () {
    const {
      removeOtherDevice,
      removeMainDevice,
      viewSyncCode,
      addDevice,
      resetSync
    } = this.state
    return (
      <EnabledContent>
        <Main>
          {removeOtherDevice ? (
            <RemoveOtherDevice
              onClose={this.onClickRemoveOtherDeviceButton}
              otherDeviceName={this.otherDeviceName}
            />
          ) : null}
          {removeMainDevice ? (
            <RemoveMainDevice
              onClose={this.onClickRemoveMainDeviceButton}
              mainDeviceName={this.mainDeviceName}
            />
          ) : null}
          {viewSyncCode ? (
            <ViewSyncCodeModal onClose={this.onClickViewSyncCodeButton} />
          ) : null}
          {addDevice ? (
            <DeviceTypeModal onClose={this.onClickAddDeviceButton} />
          ) : null}
          {resetSync ? (
            <ResetSyncModal
              onClose={this.onClickResetSyncButton}
              mainDeviceName={this.mainDeviceName}
            />
          ) : null}
          <SyncCard>
            <SyncCardContent>
              <Title level={2}>{getLocale('braveSync')}</Title>
              <Paragraph>{getLocale('syncChainDevices')}</Paragraph>
              <SectionBlock>
                <TableGrid isDeviceTable={true}>
                  <Table header={this.deviceHeader} rows={this.deviceRows} />
                  <TableButtonGrid>
                    <br />
                    <Button
                      level='secondary'
                      type='accent'
                      size='medium'
                      text={getLocale('viewSyncCode')}
                      onClick={this.onClickViewSyncCodeButton}
                    />
                    <Button
                      level='primary'
                      type='accent'
                      size='medium'
                      text={getLocale('addDevice')}
                      onClick={this.onClickAddDeviceButton}
                    />
                  </TableButtonGrid>
                </TableGrid>
              </SectionBlock>
              <Title level={2}>{getLocale('syncSettings')}</Title>
              <Paragraph>{getLocale('syncSettingsDescription')}</Paragraph>
              <SectionBlock>
                <Table header={this.settingsHeader} rows={this.settingsRows} />
              </SectionBlock>
              <SectionBlock>
                <Button
                  level='primary'
                  type='accent'
                  size='medium'
                  text={getLocale('leaveSyncChain')}
                  onClick={this.onClickResetSyncButton}
                />
              </SectionBlock>
            </SyncCardContent>
          </SyncCard>
        </Main>
      </EnabledContent>
    )
  }
}
