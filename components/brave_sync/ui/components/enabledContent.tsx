/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button } from 'brave-ui'
import SwitchButton from 'brave-ui/old/switchButton'
import Table, { Cell, Row } from 'brave-ui/components/dataTables/table'

// Feature-specific components
import {
  Grid,
  SwitchLabel,
  Paragraph,
  SectionBlock,
  SubTitle,
  TableRowId,
  TableRowDevice,
  TableRowRemove,
  TableRowRemoveButton
} from 'brave-ui/features/sync'

// Modals
import SyncANewDeviceModal from './modals/syncNewDevice'
import ResetSyncModal from './modals/resetSync'

// Utils
import { getLocale } from '../../../common/locale'

interface SyncEnabledContentProps {
  syncData: Sync.State
  actions: any
}

interface SyncEnabledContentState {
  syncANewDevice: boolean
  resetSync: boolean
}

class SyncEnabledContent extends React.PureComponent<SyncEnabledContentProps, SyncEnabledContentState> {
  constructor (props: SyncEnabledContentProps) {
    super(props)
    this.state = {
      syncANewDevice: false,
      resetSync: false
    }
  }

  getRows = (devices?: any): Row[] | undefined => {
    if (!devices) {
      return
    }

    return devices.map((device: any): Row => {
      const cell: Row = {
        content: [
          { content: <TableRowId>{device.id}</TableRowId> },
          { content: <TableRowDevice>{device.name}</TableRowDevice> },
          { content: device.lastActive },
          {
            content: (
              <TableRowRemoveButton data-id={device.id} data-name={device.name} onClick={this.onRemoveDevice}>
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
      { content: <TableRowId>{getLocale('id')}</TableRowId> },
      { content: <TableRowDevice>{getLocale('deviceName')}</TableRowDevice> },
      { content: getLocale('lastActive') },
      { content: <TableRowRemove>{getLocale('removeDevice')}</TableRowRemove> }
    ]
  }

  onRemoveDevice = (event: React.MouseEvent<HTMLSpanElement>) => {
    const target = event.target as HTMLSpanElement
    this.props.actions.onRemoveDevice(Number(target.dataset.id), target.dataset.name)
  }

  syncANewDeviceModal = () => {
    this.setState({ syncANewDevice: !this.state.syncANewDevice })
  }

  resetSyncModal = () => {
    this.setState({ resetSync: !this.state.resetSync })
  }

  onSyncReset = () => {
    if (window.confirm(getLocale('areYouSure'))) {
      this.props.actions.onSyncReset()
    }
  }

  onSyncBookmarks = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.onSyncBookmarks(event.target.checked)
  }

  onSyncSavedSiteSettings = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.onSyncSavedSiteSettings(event.target.checked)
  }

  onSyncBrowsingHistory = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.onSyncBrowsingHistory(event.target.checked)
  }

  render () {
    const { actions, syncData } = this.props
    return (
      <>
        {
          this.state.syncANewDevice
            ? (
              <SyncANewDeviceModal
                actions={actions}
                seedQRImageSource={syncData.seedQRImageSource}
                syncWords={syncData.syncWords}
                onClose={this.syncANewDeviceModal}
              />
            )
            : null
        }
        {
          this.state.resetSync
            ? <ResetSyncModal actions={actions} onClose={this.resetSyncModal} />
            : null
        }
        <SectionBlock>
          <SubTitle level={2}>{getLocale('devices')}</SubTitle>
          <Table header={this.header} rows={this.getRows(syncData.devices)}>
            Device list is empty
          </Table>
          <Button
            level='primary'
            type='accent'
            size='medium'
            text={getLocale('syncANewDevice')}
            onClick={this.syncANewDeviceModal}
          />
        </SectionBlock>
        <SectionBlock>
          <SubTitle level={2}>{getLocale('syncData')}</SubTitle>
          <Paragraph>{getLocale('syncDataInfo')}</Paragraph>
          <Grid columns='auto 1fr' rows='1fr 1fr 1fr' gap='5px'>
            <SwitchButton
              id='bookmarks'
              checked={syncData.syncBookmarks}
              onChange={this.onSyncBookmarks}
            />
            <SwitchLabel htmlFor='bookmarks'>
              {getLocale('bookmarks')}
            </SwitchLabel>
            <SwitchButton
              id='savedSiteSettings'
              checked={syncData.syncSavedSiteSettings}
              onChange={this.onSyncSavedSiteSettings}
              disabled={true}
            />
            <SwitchLabel htmlFor='savedSiteSettings'>
              {getLocale('savedSiteSettings')}
            </SwitchLabel>
            <SwitchButton
              id='browsingHistory'
              checked={syncData.syncBrowsingHistory}
              onChange={this.onSyncBrowsingHistory}
              disabled={true}
            />
            <SwitchLabel htmlFor='browsingHistory'>
              {getLocale('browsingHistory')}
            </SwitchLabel>
          </Grid>
        </SectionBlock>
        <SectionBlock>
          <SubTitle level={2}>{getLocale('clearData')}</SubTitle>
          <Button
            level='primary'
            type='accent'
            size='medium'
            text={getLocale('resetSync')}
            onClick={this.onSyncReset}
          />
        </SectionBlock>
      </>
    )
  }
}

export default SyncEnabledContent
