/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import { LocaleContext } from '../../shared/lib/locale_context'
import { HostContext } from '../lib/host_context'

import * as style from './self_custody_invite.style'

export function SelfCustodyInvite () {
  const host = React.useContext(HostContext)
  const { getString } = React.useContext(LocaleContext)

  const onConnect = () => {
    host.dismissSelfCustodyInvite()
    host.handleExternalWalletAction('verify')
  }

  return (
    <style.root>
      <style.header>
        <style.close>
          <button onClick={host.dismissSelfCustodyInvite}>
            <Icon name='close' />
          </button>
        </style.close>
        {getString('rewardsSelfCustodyInviteHeader')}
      </style.header>
      <style.text>
        {getString('rewardsSelfCustodyInviteText')}
      </style.text>
      <Button onClick={onConnect}>
        <style.connect>
          {getString('rewardsConnectAccount')}
          <Icon name='arrow-right' />
        </style.connect>
      </Button>
      <style.dismiss>
        <button onClick={host.dismissSelfCustodyInvite}>
          {getString('rewardsNotNow')}
        </button>
      </style.dismiss>
    </style.root>
  )
}
