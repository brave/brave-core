/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import SegmentedControl from '@brave/leo/react/segmentedControl'
import SegmentedControlItem from '@brave/leo/react/segmentedControlItem'

import { useTransferApi } from '../api/transfer_api_context'
import { getString } from '../lib/strings'
import { DevicesTab } from './devices_tab'
import { InboxTab } from './inbox_tab'
import { PermissionsCard, SyncOffNotice } from './notices'
import { OfferList } from './offer_list'
import { SettingsTab } from './settings_tab'

import { style } from './app.style'

export type Tab = 'devices' | 'inbox' | 'settings'

interface Props {
  initialTab?: Tab
}

export function App(props: Props) {
  const api = useTransferApi()
  const isIOS = api.useIsIOSData()
  const isMobile = api.useIsMobileData()
  const { syncEnabled } = api.useGetServiceStateData()
  const [tab, setTab] = React.useState<Tab>(props.initialTab ?? 'devices')

  return (
    <main
      data-css-scope={style.scope}
      data-mobile={isMobile}
    >
      <div className='content'>
        <header>
          <div className='title'>
            <Icon name='smartphone-laptop' />
            <h1>{getString('BRAVE_TRANSFER_TITLE')}</h1>
          </div>
          <p className='description'>
            {getString('BRAVE_TRANSFER_DESCRIPTION')}
          </p>
          <p className='note'>
            {getString(
              isIOS
                ? 'BRAVE_TRANSFER_IOS_FOREGROUND_NOTE'
                : 'BRAVE_TRANSFER_DESKTOP_TAB_NOTE',
            )}
          </p>
        </header>
        {!syncEnabled && <SyncOffNotice />}
        {isIOS && tab !== 'settings' && <PermissionsCard />}
        <OfferList />
        <SegmentedControl
          className='tabs'
          value={tab}
          onChange={({ value }) => setTab((value as Tab) ?? 'devices')}
        >
          <SegmentedControlItem value='devices'>
            {getString('BRAVE_TRANSFER_TAB_DEVICES')}
          </SegmentedControlItem>
          <SegmentedControlItem value='inbox'>
            {getString('BRAVE_TRANSFER_TAB_INBOX')}
          </SegmentedControlItem>
          <SegmentedControlItem value='settings'>
            {getString('BRAVE_TRANSFER_TAB_SETTINGS')}
          </SegmentedControlItem>
        </SegmentedControl>
        {tab === 'devices' && <DevicesTab />}
        {tab === 'inbox' && <InboxTab />}
        {tab === 'settings' && <SettingsTab />}
      </div>
    </main>
  )
}
