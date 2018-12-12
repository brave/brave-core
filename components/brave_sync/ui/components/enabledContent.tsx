/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button } from 'brave-ui'
import Table, { Cell, Row } from 'brave-ui/components/dataTables/table'
import { Toggle } from 'brave-ui/features/shields'

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
} from 'brave-ui/features/sync'

// Modals
import RemoveDeviceModal from './modals/removeDevice'
import ViewSyncCodeModal from './modals/viewSyncCode'
import DeviceTypeModal from './modals/deviceType'
import ResetSyncModal from './modals/resetSync'

// Utils
import { getLocale } from '../../../common/locale'

interface Props {
  syncData: Sync.State
  actions: any
}

interface State {
  removeDevice: boolean
  viewSyncCode: boolean
  addDevice: boolean
  resetSync: boolean
  deviceToRemoveName: string | undefined
  deviceToRemoveId: string | undefined
}

export default class SyncEnabledContent extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      removeDevice: false,
      viewSyncCode: false,
      addDevice: false,
      resetSync: false,
      deviceToRemoveName: '',
      deviceToRemoveId: ''
    }
  }

   componentDidUpdate () {
    // immediately request qr code and sync words
    // in case they aren't already. this could happen if user
    // had the sync word where the requests are stopped due to sync reset
    const { seedQRImageSource, syncWords } = this.props.syncData
    if (!seedQRImageSource && !syncWords) {
       this.props.actions.onRequestQRCode()
       this.props.actions.onRequestSyncWords()
    }
  }

  getRows = (devices?: any): Row[] | undefined => {
    if (!devices) {
      return
    }

    return devices.map((device: any): Row => {
      const cell: Row = {
        content: [
          { content:
            <TableRowDevice>
              {device.name} {Number(device.id) === 0 ? getLocale('mainDevice') : null }
            </TableRowDevice>
          },
          { content: device.lastActive },
          {
            content: (
              <TableRowRemoveButton
                data-id={device.id}
                data-name={device.name}
                data-main={device.thisDeviceName}
                onClick={this.onClickRemoveDeviceButton}
              >
                &times;
              </TableRowRemoveButton>
            )
          }
        ]
      }
      return cell
    })
  }

  get header (): Cell[] {
    return [
      { content: <TableRowDevice>{getLocale('deviceName')}</TableRowDevice> },
      { content: getLocale('addedOn') },
      { content: <TableRowRemove>{getLocale('remove')}</TableRowRemove> }
    ]
  }

  onClickRemoveDeviceButton = (event: React.MouseEvent<HTMLButtonElement>) => {
    if (!event || !event.currentTarget || !event.currentTarget.dataset) {
      return
    }
    const target = event.currentTarget as HTMLButtonElement
    this.setState({
      deviceToRemoveName: target.dataset.name,
      deviceToRemoveId: target.dataset.id,
      removeDevice: !this.state.removeDevice
    })
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

  onSyncBookmarks = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.onSyncBookmarks(event.target.checked)
  }

  render () {
    const { actions, syncData } = this.props
    const {
      removeDevice,
      viewSyncCode,
      addDevice,
      resetSync,
      deviceToRemoveName,
      deviceToRemoveId
    } = this.state

    if (!syncData) {
      return null
    }

    return (
      <Main>
        {
          removeDevice
            ? <RemoveDeviceModal
              deviceName={deviceToRemoveName}
              deviceId={Number(deviceToRemoveId)}
              actions={actions}
              onClose={this.onClickRemoveDeviceButton}
              />
            : null
        }
        {
          viewSyncCode
            ? <ViewSyncCodeModal syncData={syncData} actions={actions} onClose={this.onClickViewSyncCodeButton} />
            : null
        }
        {
          addDevice
            ? <DeviceTypeModal syncData={syncData} actions={actions} onClose={this.onClickAddDeviceButton} />
            : null
        }
        {
          resetSync
            ? <ResetSyncModal syncData={syncData} actions={actions} onClose={this.onClickResetSyncButton} />
            : null
        }
        <Title level={2}>{getLocale('braveSync')}</Title>
        <SectionBlock>
          <SubTitle level={2}>{getLocale('syncChainDevices')}</SubTitle>
          <TableGrid>
            <Table header={this.header} rows={this.getRows(syncData.devices)}>
              Device list is empty
            </Table>
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
          <SubTitle level={2}>{getLocale('dataToSync')} {syncData.thisDeviceName}</SubTitle>
          <SettingsToggleGrid>
            <Toggle
              id='bookmarks'
              checked={syncData.syncBookmarks}
              onChange={this.onSyncBookmarks}
            />
            <SwitchLabel htmlFor='bookmarks'>
              {getLocale('bookmarks')}
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
