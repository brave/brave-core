/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import createWidget, { WidgetProps } from '../widget/index'
import { StyledCard, StyledTitleTab } from '../widgetCard'
import { VPNMainWidget, VPNPromoWidget, VPNWidgetTitle } from '../vpn/vpn_card'
import { BraveVPNState } from 'components/brave_new_tab_ui/reducers/brave_vpn'
import * as BraveVPN from '../../../api/braveVpn'
import { useNewTabPref } from '../../../hooks/usePref'

export interface VPNProps {
  showContent: boolean
  onShowContent: () => void
  braveVPNState: BraveVPNState
}

const VPNWidgetInternal = createWidget((props: VPNProps) => {
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
        <VPNMainWidget
          connectionState={props.braveVPNState.connectionState}
          selectedRegion={props.braveVPNState.selectedRegion}
        />
      ) : (
        <VPNPromoWidget />
      )}
    </StyledCard>
  )
})

export const VPNWidget = (props: WidgetProps & VPNProps) => {
  const [, saveShowBraveVPN] = useNewTabPref('showBraveVPN')

  return (
    <VPNWidgetInternal
      {...props}
      hideWidget={() => saveShowBraveVPN(false)}
    />
  )
}
