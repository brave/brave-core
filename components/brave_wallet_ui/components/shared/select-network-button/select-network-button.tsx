// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Components
import { CreateNetworkIcon } from '../create-network-icon/index'

// Styled Components
import {
  OvalButton,
  OvalButtonText,
  CaratDownIcon
} from './select-network-button.style'

export interface Props {
  onClick?: () => void
  selectedNetwork?: BraveWallet.NetworkInfo
  isPanel?: boolean
}

export const SelectNetworkButton = ({
  onClick,
  selectedNetwork,
  isPanel
}: Props) => {
  return (
    <OvalButton isPanel={isPanel} onClick={onClick} data-test-id='select-network-button'>
      <CreateNetworkIcon network={selectedNetwork} marginRight={4} />
      <OvalButtonText isPanel={isPanel}>{selectedNetwork?.chainName ?? ''}</OvalButtonText>
      {onClick && <CaratDownIcon isPanel={isPanel} />}
    </OvalButton>
  )
}

export default SelectNetworkButton
