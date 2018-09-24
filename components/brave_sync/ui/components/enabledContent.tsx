/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Card, Toggle, Button } from 'brave-ui'
import Table, { Cell, Row } from 'brave-ui/components/dataTables/table'

// Feature-specific components
import {
  Grid,
  FlexColumn,
  SwitchLabel,
  Label,
  Paragraph,
  SectionBlock,
  SubTitle
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

  get rows (): Row[] {
    return [
      {
        content: [
          { content: 1 },
          { content: 'MacOS without the ESC key' },
          { content: '6/12/2018, 12:10:16 PM' }
        ]
      },
      {
        content: [
          { content: 2 },
          { content: 'Windowz machineh' },
          { content: '8/1/2018, 7:12:32 PM' }
        ]
      }
    ]
  }

  get header (): Cell[] {
    return [
      { content: getLocale('id') },
      { content: getLocale('deviceName') },
      { content: getLocale('lastActive') }
    ]
  }

  syncANewDevice = () => {
    this.setState({ syncANewDevice: !this.state.syncANewDevice })
  }

  resetSync = () => {
    this.setState({ resetSync: !this.state.resetSync })
  }

  render () {
    return (
      <>
        {
          this.state.syncANewDevice
            ? <SyncANewDeviceModal onClose={this.syncANewDevice} />
            : null
        }
        {
          this.state.resetSync
            ? <ResetSyncModal onClose={this.resetSync} />
            : null
        }
        <Card>
          <Grid columns='1fr 1fr'>
            <FlexColumn items='center'>
              <Toggle id='syncThisDevice' size='large' checked={false} />
              <SwitchLabel htmlFor='syncThisDevice'>{getLocale('syncThisDevice')}</SwitchLabel>
            </FlexColumn>
            <FlexColumn direction='column'>
              <Label>{getLocale('deviceName')}</Label>
              <Paragraph>MacOS without the ESC key</Paragraph>
            </FlexColumn>
          </Grid>
        </Card>
        <SectionBlock>
          <SubTitle level={2}>{getLocale('devices')}</SubTitle>
          <Table header={this.header} rows={this.rows} />
          <Button
            level='primary'
            type='accent'
            size='medium'
            text={getLocale('syncANewDevice')}
            onClick={this.syncANewDevice}
          />
        </SectionBlock>
        <SectionBlock>
          <SubTitle level={2}>{getLocale('syncData')}</SubTitle>
          <Paragraph>{getLocale('syncDataInfo')}</Paragraph>
          <Grid columns='auto 1fr' rows='1fr 1fr 1fr' gap='5px'>
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
          </Grid>
        </SectionBlock>
        <SectionBlock>
          <SubTitle level={2}>{getLocale('clearData')}</SubTitle>
          <Button
            level='primary'
            type='accent'
            size='medium'
            text={getLocale('resetSync')}
            onClick={this.resetSync}
          />
        </SectionBlock>
      </>
    )
  }
}

export default SyncEnabledContent
