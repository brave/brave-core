// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Hooks
import { useCopyToClipboard } from '../../../../../common/hooks/use-copy-to-clipboard'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Styled Components
import { AddressButton } from './copy-address.style'
import { Row, Text } from '../../shared.styles'

interface Props {
  address: string
}

export const CopyAddress = (props: Props) => {
  const { address } = props

  // Hooks
  const { isCopied, copyToClipboard } = useCopyToClipboard(1500)

  // Methods
  const handleClick = React.useCallback(async () => {
    await copyToClipboard(address)
  }, [address])

  return (
    <Row
      rowWidth='full'
      paddingBottom={16}
      horizontalPadding={16}
      horizontalAlign='flex-start'
    >
      <AddressButton onClick={handleClick}>
        <Text
          textSize='12px'
          textColor='text03'
          isBold={false}
        >
          {address}
        </Text>
      </AddressButton>
      {isCopied &&
        <Text
          textSize='12px'
          textColor='success'
          isBold={false}
        >
          {getLocale('braveWalletButtonCopied')}
        </Text>
      }
    </Row>
  )
}

export default CopyAddress
