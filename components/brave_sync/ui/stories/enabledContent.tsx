/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Toggle from '../../../src/components/formControls/toggle'
import Button from '../../../src/components/buttonsIndicators/button'
import Table, { Cell, Row } from '../../../src/components/dataTables/table'

// Feature-specific components
import {
  Main,
  Title,
  SettingsToggleGrid,
  SwitchLabel,
  SectionBlock,
  SubTitle,
  TableRowDevice,
  TableRowRemove,
  TableRowRemoveButton,
  TableGrid,
  TableButtonGrid
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

  get rows (): Row[] {
    return [
      {
        content: [
          { content: <TableRowDevice>{data.device1.name} (main device)</TableRowDevice> },
          { content: data.device1.lastActive },
          {
            content: (
              <TableRowRemoveButton data-id={''} data-name={''} onClick={this.onClickRemoveMainDeviceButton}>
                &times;
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
              <TableRowRemoveButton data-id={''} data-name={''} onClick={this.onClickRemoveOtherDeviceButton}>
                &times;
              </TableRowRemoveButton>
            )
          }
        ]
      }
    ]
  }

  get header (): Cell[] {
    return [
      { content: <TableRowDevice>{getLocale('deviceName')}</TableRowDevice> },
      { content: getLocale('addedOn') },
      { content: <TableRowRemove>{getLocale('remove')}</TableRowRemove> }
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
    const { removeOtherDevice, removeMainDevice, viewSyncCode, addDevice, resetSync } = this.state
    return (
      <Main>
        {
          removeOtherDevice
            ? <RemoveOtherDevice onClose={this.onClickRemoveOtherDeviceButton} otherDeviceName={this.otherDeviceName} />
            : null
        }
        {
          removeMainDevice
            ? <RemoveMainDevice onClose={this.onClickRemoveMainDeviceButton} mainDeviceName={this.mainDeviceName} />
            : null
        }
        {
          viewSyncCode
            ? <ViewSyncCodeModal onClose={this.onClickViewSyncCodeButton} />
            : null
        }
        {
          addDevice
            ? <DeviceTypeModal onClose={this.onClickAddDeviceButton} mainDeviceName={this.mainDeviceName} />
            : null
        }
        {
          resetSync
            ? <ResetSyncModal onClose={this.onClickResetSyncButton} mainDeviceName={this.mainDeviceName} />
            : null
        }
        <Title level={2}>{getLocale('braveSync')}</Title>
        <SectionBlock>
          <SubTitle level={2}>{getLocale('syncChainDevices')}</SubTitle>
          <TableGrid>
            <Table header={this.header} rows={this.rows} />
            <TableButtonGrid>
              <Button
                level='secondary'
                type='accent'
                size='medium'
                text={getLocale('addDevice')}
                onClick={this.onClickAddDeviceButton}
              />
              <Button
                level='secondary'
                type='accent'
                size='medium'
                text={getLocale('viewSyncCode')}
                onClick={this.onClickViewSyncCodeButton}
              />
            </TableButtonGrid>
          </TableGrid>
        </SectionBlock>
        <SectionBlock>
          <SubTitle level={2}>{getLocale('dataToSync')} {data.device1.name}</SubTitle>
          <SettingsToggleGrid>
            <Toggle id='bookmarks' checked={false} />
            <SwitchLabel htmlFor='bookmarks'>
              {getLocale('bookmarks')}
            </SwitchLabel>
            <Toggle id='savedSiteSettings' checked={false} />
            <SwitchLabel htmlFor='savedSiteSettings'>
              {getLocale('savedSiteSettings')}
            </SwitchLabel>
            <Toggle id='browsingHistory' checked={false} />
            <SwitchLabel htmlFor='browsingHistory'>
              {getLocale('browsingHistory')}
            </SwitchLabel>
          </SettingsToggleGrid>
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
      </Main>
    )
  }
}
