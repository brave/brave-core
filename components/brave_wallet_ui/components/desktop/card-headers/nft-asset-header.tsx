// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  MenuButton,
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

export const NftAssetHeader = ({
  assetName,
  tokenId,
  showSendButton,
  onBack,
  onSend
}: Props) => {
  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  return (
    <Row
      padding={isPanel ? '12px 20px' : '26px 0px'}
      justifyContent='space-between'
      alignItems='center'
    >
      <Row
        justifyContent='flex-start'
        margin='0px 6px 0px 0px'
      >
        <MenuButton
          marginRight={16}
          onClick={onBack}
        >
          <ButtonIcon
            size={16}
            name='arrow-left'
          />
        </MenuButton>
        <HeaderTitle isPanel={isPanel}>
          {assetName}&nbsp;{tokenId ? `#${new Amount(tokenId).toNumber()}` : ''}
        </HeaderTitle>
      </Row>
      {showSendButton && (
        <SendButton onClick={onSend}>{getLocale('braveWalletSend')}</SendButton>
      )}
    </Row>
  )
}

export default NftAssetHeader
