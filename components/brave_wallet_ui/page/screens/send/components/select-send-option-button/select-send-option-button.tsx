// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { SendPageTabHashes } from '../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Styled Components
import { ButtonsContainer, Button } from './select-send-option-button.style'

interface Props {
  selectedSendOption: SendPageTabHashes
  onClick: (sendOption: SendPageTabHashes) => void
}

export const SelectSendOptionButton = (props: Props) => {
  const { onClick, selectedSendOption } = props

  return (
    <ButtonsContainer>
      <Button
        isSelected={selectedSendOption === SendPageTabHashes.token}
        onClick={() => onClick('#token')}
        buttonAlign='left'
      >
        {getLocale('braveWalletSendToken')}
      </Button>
      <Button
        isSelected={selectedSendOption === SendPageTabHashes.nft}
        onClick={() => onClick('#nft')}
        buttonAlign='right'
      >
        {getLocale('braveWalletSendNFT')}
      </Button>
    </ButtonsContainer>
  )
}

export default SelectSendOptionButton
