// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  CircleButton,
  HeaderTitle,
  ButtonIcon,
  SendButton
} from './shared-card-headers.style'
import { Row } from '../../shared/style'

interface Props {
  assetName?: string
  tokenId?: string
  showSendButton: boolean
  onBack: () => void
  onSend: () => void
}

export const NftAssetHeader = ({ assetName, tokenId, showSendButton, onBack, onSend }: Props) => {
  return (
    <Row
      padding='26px 0px'
      justifyContent='space-between'
      alignItems='center'
    >
      <Row
        justifyContent='flex-start'
      >
        <CircleButton
          size={28}
          marginRight={16}
          onClick={onBack}
        >
          <ButtonIcon
            size={16}
            name='arrow-left'
          />
        </CircleButton>
        <HeaderTitle>
          {assetName}&nbsp;{tokenId ? `#${new Amount(tokenId).toNumber()}` : ''}
        </HeaderTitle>
      </Row>
      {showSendButton && <SendButton onClick={onSend}>{getLocale('braveWalletSend')}</SendButton>}
    </Row>
  )
}

export default NftAssetHeader
