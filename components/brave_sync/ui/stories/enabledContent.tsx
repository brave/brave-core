/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Card from '../../../src/components/layout/card'
import Toggle from '../../../src/components/formControls/toggle'
import Button from '../../../src/components/buttonsIndicators/button'
import Table, { Cell, Row } from '../../../src/components/dataTables/table'

// Feature-specific components
import {
  Grid,
  FlexColumn,
  SwitchLabel,
  Label,
  Paragraph,
  SectionBlock,
  SubTitle,
  TableRowId,
  TableRowDevice,
  TableRowRemove,
  TableRowRemoveButton
} from '../../../src/features/sync'

// Modals
import SyncANewDeviceModal from './modals/syncNewDevice'
import ResetSyncModal from './modals/resetSync'

// Utils
import locale from './page/fakeLocale'
import data from './page/fakeData'

interface SyncEnabledContentState {
  syncANewDevice: boolean
  resetSync: boolean
}

class SyncEnabledContent extends React.PureComponent<{}, SyncEnabledContentState> {
  constructor (props: {}) {
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
          { content: <TableRowId>{data.device1.id}</TableRowId> },
          { content: <TableRowDevice>{data.device1.name}</TableRowDevice> },
          { content: data.device1.lastActive },
          {
            content: (
              <TableRowRemoveButton data-id={''} data-name={''}>
                &times;
              </TableRowRemoveButton>
            )
          }
        ]
      },
      {
        content: [
          { content: <TableRowId>{data.device2.id}</TableRowId> },
          { content: <TableRowDevice>{data.device2.name}</TableRowDevice> },
          { content: data.device2.lastActive },
          {
            content: (
              <TableRowRemoveButton data-id={''} data-name={''}>
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
      { content: <TableRowId>{locale.id}</TableRowId> },
      { content: <TableRowDevice>{locale.deviceName}</TableRowDevice> },
      { content: locale.lastActive },
      { content: <TableRowRemove>{locale.removeDevice}</TableRowRemove> }
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
              <SwitchLabel htmlFor='syncThisDevice'>{locale.syncThisDevice}</SwitchLabel>
            </FlexColumn>
            <FlexColumn direction='column'>
              <Label>{locale.deviceName}</Label>
              <Paragraph>MacOS without the ESC key</Paragraph>
            </FlexColumn>
          </Grid>
        </Card>
        <SectionBlock>
          <SubTitle level={2}>{locale.devices}</SubTitle>
          <Table header={this.header} rows={this.rows} />
          <Button
            level='primary'
            type='accent'
            size='medium'
            text={locale.syncANewDevice}
            onClick={this.syncANewDevice}
          />
        </SectionBlock>
        <SectionBlock>
          <SubTitle level={2}>{locale.syncData}</SubTitle>
          <Paragraph>{locale.syncDataInfo}</Paragraph>
          <Grid columns='auto 1fr' rows='1fr 1fr 1fr' gap='5px'>
            <Toggle id='bookmarks' checked={false} />
            <SwitchLabel htmlFor='bookmarks'>
              {locale.bookmarks}
            </SwitchLabel>
            <Toggle id='savedSiteSettings' checked={false} />
            <SwitchLabel htmlFor='savedSiteSettings'>
              {locale.savedSiteSettings}
            </SwitchLabel>
            <Toggle id='browsingHistory' checked={false} />
            <SwitchLabel htmlFor='browsingHistory'>
              {locale.browsingHistory}
            </SwitchLabel>
          </Grid>
        </SectionBlock>
        <SectionBlock>
          <SubTitle level={2}>{locale.clearData}</SubTitle>
          <Button
            level='primary'
            type='accent'
            size='medium'
            text={locale.resetSync}
            onClick={this.resetSync}
          />
        </SectionBlock>
      </>
    )
  }
}

export default SyncEnabledContent
