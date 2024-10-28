/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import createWidget from '../widget/index'
import { StyledCard, StyledTitleTab } from '../widgetCard'
import { VPNWidget as VPNCard, VPNWidgetHeader as VPNCardHeader} from '../vpn/vpn_card'
import { BraveVPNState } from "components/brave_new_tab_ui/reducers/brave_vpn";

export interface VPNProps {
  showContent: boolean
  onShowContent: () => void
  braveVPNState: BraveVPNState
}

export const VPNWidget = createWidget((props: VPNProps) => {
  if (!props.showContent) {
    return (
      <StyledTitleTab onClick={props.onShowContent}>
          <VPNCardHeader />
      </StyledTitleTab>
    )
  }
  return (
    <StyledCard>
      <VPNCard {...props.braveVPNState} />
    </StyledCard>
  )
})
