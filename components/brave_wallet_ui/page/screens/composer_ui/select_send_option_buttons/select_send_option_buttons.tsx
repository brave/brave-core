// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { SendPageTabHashes } from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'

// Styled Components
import { Row } from '../../../../components/shared/style'
import {
  Button,
  ButtonText,
  LineIndicator
} from './select_send_option_buttons.style'

interface Props {
  selectedSendOption: SendPageTabHashes
  onClick: (sendOption: SendPageTabHashes) => void
}

export const SelectSendOptionButtons = (props: Props) => {
  const { onClick, selectedSendOption } = props

  return (
    <Row width='unset'>
      <Button
        isSelected={selectedSendOption === SendPageTabHashes.token}
        onClick={() => onClick('#token')}
      >
        <ButtonText>{getLocale('braveWalletAccountsAssets')}</ButtonText>
        <LineIndicator
          isSelected={selectedSendOption === SendPageTabHashes.token}
        />
      </Button>
      <Button
        isSelected={selectedSendOption === SendPageTabHashes.nft}
        onClick={() => onClick('#nft')}
      >
        <ButtonText>{getLocale('braveWalletTopNavNFTS')}</ButtonText>
        <LineIndicator
          isSelected={selectedSendOption === SendPageTabHashes.nft}
        />
      </Button>
    </Row>
  )
}
