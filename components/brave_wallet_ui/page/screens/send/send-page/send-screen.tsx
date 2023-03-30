// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { SendOptionTypes } from '../../../../constants/types'

// Hooks
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'

// Components
import { Send } from '../send/send'
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
    <>
      <Send
        onShowSelectTokenModal={() => setShowSelectTokenModal(true)}
        onHideSelectTokenModal={() => setShowSelectTokenModal(false)}
        selectedSendOption={selectedSendOption}
        setSelectedSendOption={setSelectedSendOption}
        showSelectTokenModal={showSelectTokenModal}
        selectTokenModalRef={selectTokenModalRef}
      />
      <FeatureRequestButton />
    </>
  )
}

export default SendScreen
