/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useDispatch } from 'react-redux'

import createWidget from '../widget/index'
import { StyledCard, StyledTitleTab } from '../widgetCard'
import {
  VPNWidget as VPNCard,
  VPNPromoWidget as VPNPromoCard,
  VPNWidgetTitle
} from '../vpn/vpn_card'
import { BraveVPNState } from 'components/brave_new_tab_ui/reducers/brave_vpn'
import * as BraveVPN from '../../../api/braveVpn'
import * as Actions from '../../../actions/brave_vpn_actions'

export interface VPNProps {
  showContent: boolean
  onShowContent: () => void
  braveVPNState: BraveVPNState
}

export const VPNWidget = createWidget((props: VPNProps) => {
  const dispatch = useDispatch()

  // TODO(simonhong): Delete this.
  // Initialize from c++ and only observer this
  React.useEffect(() => {
    dispatch(Actions.initialize())
  }, [])

  if (!props.showContent) {
    return (
      <StyledTitleTab onClick={props.onShowContent}>
        <VPNWidgetTitle />
      </StyledTitleTab>
    )
  }

  return (
    <StyledCard>
      {props.braveVPNState.purchasedState ===
      BraveVPN.PurchasedState.PURCHASED ? (
        <VPNCard {...props.braveVPNState} />
      ) : (
        <VPNPromoCard />
      )}
    </StyledCard>
  )
})
