/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext } from '../lib/host_context'
import { LocaleContext } from '../../shared/lib/locale_context'
import { ArrowNextIcon } from '../../shared/components/icons/arrow_next_icon'

import tipCreatorsGraphic from '../assets/tip_creators.svg'

import * as urls from '../../shared/lib/rewards_urls'
import * as style from './limited_tip_form.style'

export function LimitedTipForm () {
  const host = React.useContext(HostContext)
  const locale = React.useContext(LocaleContext)
  const { getString } = locale

  const [balanceInfo, setBalanceInfo] = React.useState(
    host.state.balanceInfo)
  const [publisherInfo, setPublisherInfo] = React.useState(
    host.state.publisherInfo)
  const [rewardsParameters, setRewardsParameters] = React.useState(
    host.state.rewardsParameters)
  const [walletInfo, setWalletInfo] = React.useState(
    host.state.externalWalletInfo)

  React.useEffect(() => {
    return host.addListener((state) => {
      setRewardsParameters(state.rewardsParameters)
      setPublisherInfo(state.publisherInfo)
      setBalanceInfo(state.balanceInfo)
      setWalletInfo(state.externalWalletInfo)
    })
  }, [host])

  function onConnectAccount () {
    window.open(urls.connectURL, '_blank')
    host.closeDialog()
  }

  if (!rewardsParameters || !publisherInfo || !balanceInfo || !walletInfo) {
    return <style.loading />
  }

  return (
    <style.root>
      <style.header>
        {getString('tipFavoriteCreators')}
        <style.headerSubtitle>
          {getString('connectAccountText')}
        </style.headerSubtitle>
      </style.header>
      <style.tipCreatorsGraphic>
        <img src={tipCreatorsGraphic} />
      </style.tipCreatorsGraphic>
      <style.connectAction>
        <button onClick={onConnectAccount}>
          {getString('connectAccountButton')}<ArrowNextIcon />
        </button>
      </style.connectAction>
    </style.root>
  )
}
