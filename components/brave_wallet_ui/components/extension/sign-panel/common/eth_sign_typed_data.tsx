// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Utils
import { getLocale } from '../../../../../common/locale'
import { formatTypedDataForDisplay } from '../../../../utils/string-utils'

// Types
import { BraveWallet } from '../../../../constants/types'

// Styled Components
import { MessageHeader, MessageText } from '../style'
import { SignMessageBox } from './sign_message_box'
import { SignMessageCharacterWarning } from './sign_message_character_warning'

interface Props {
  data?: BraveWallet.EthSignTypedData
  height?: string
  width?: string
}

export function EthSignTypedData(props: Props) {
  const { data, height, width } = props

  const [showFormatted, setShowFormatted] = React.useState(true)

  return (
    <>
      <SignMessageCharacterWarning
        messages={[data?.domainJson, data?.messageJson]}
        showFormatted={showFormatted}
        onToggle={() => setShowFormatted((current) => !current)}
        decodeJson={true}
      />

      {data && (
        <SignMessageBox
          height={height ?? '180px'}
          width={width}
        >
          <MessageHeader
            textColor='secondary'
            variant='small.semibold'
          >
            {getLocale(S.BRAVE_WALLET_SIGN_TRANSACTION_EIP712_MESSAGE_DOMAIN)}:
          </MessageHeader>
          <MessageText
            textColor='secondary'
            variant='small.regular'
          >
            {formatTypedDataForDisplay(data.domainJson, showFormatted)}
          </MessageText>

          <MessageHeader
            textColor='secondary'
            variant='small.semibold'
          >
            {getLocale(S.BRAVE_WALLET_SIGN_TRANSACTION_MESSAGE_TITLE)}:
          </MessageHeader>
          <MessageText
            textColor='secondary'
            variant='small.regular'
          >
            {formatTypedDataForDisplay(data.messageJson, showFormatted)}
          </MessageText>
        </SignMessageBox>
      )}
    </>
  )
}
