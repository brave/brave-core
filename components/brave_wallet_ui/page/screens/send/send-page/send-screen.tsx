// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { SendOptionTypes } from '../../../../constants/types'

// Hooks
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'

// Styled Components
import { SendScreenWrapper } from './send-screen.style'

// Components
import { TabHeader } from '../../shared-screen-components/tab-header/tab-header'
import { Send } from '../send/send'
import { BuySendSwapDepositNav } from '../../../../components/desktop/buy-send-swap-deposit-nav/buy-send-swap-deposit-nav'
import { FeatureRequestButton } from '../../../../components/shared/feature-request-button/feature-request-button'

export const SendScreen = () => {
  // State
  const [showSelectTokenModal, setShowSelectTokenModal] = React.useState<boolean>(false)
  const [selectedSendOption, setSelectedSendOption] = React.useState<SendOptionTypes>('token')

  // Refs
  const selectTokenModalRef = React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(
    selectTokenModalRef,
    () => setShowSelectTokenModal(false),
    showSelectTokenModal
  )

  // render
  return (
    <SendScreenWrapper>
      <TabHeader title='braveWalletSend' />
      <BuySendSwapDepositNav isTab={true} />
      <Send
        onShowSelectTokenModal={() => setShowSelectTokenModal(true)}
        onHideSelectTokenModal={() => setShowSelectTokenModal(false)}
        selectedSendOption={selectedSendOption}
        setSelectedSendOption={setSelectedSendOption}
        showSelectTokenModal={showSelectTokenModal}
        selectTokenModalRef={selectTokenModalRef}
      />
      <FeatureRequestButton />
    </SendScreenWrapper>
  )
}

export default SendScreen
