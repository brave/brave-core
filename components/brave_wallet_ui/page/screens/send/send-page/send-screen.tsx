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
import { SendHeader } from '../components/header/header'
import { Send } from '../send/send'
import { SelectTokenModal } from '../components/select-token-modal/select-token-modal'

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
      <SendHeader />
      <Send
        onShowSelectTokenModal={() => setShowSelectTokenModal(true)}
        selectedSendOption={selectedSendOption}
        setSelectedSendOption={setSelectedSendOption}
      />
      {showSelectTokenModal &&
        <SelectTokenModal
          onClose={() => setShowSelectTokenModal(false)}
          selectedSendOption={selectedSendOption}
          ref={selectTokenModalRef}
        />
      }
    </SendScreenWrapper>
  )
}

export default SendScreen
