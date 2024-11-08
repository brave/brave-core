// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Utils
import { getLocale } from '../../../../../common/locale'
import { unicodeEscape, hasUnicode } from '../../../../utils/string-utils'

// Types
import { BraveWallet } from '../../../../constants/types'

// Styled Components
import {
  MessageBox,
  MessageHeader,
  MessageText,
  WarningTitleRow
} from '../style'

import {
  WarningBox,
  WarningTitle,
  LearnMoreButton,
  WarningIcon
} from '../../shared-panel-styles'

interface Props {
  data?: BraveWallet.EthSignTypedData
  height?: string
  width?: string
}

export function EthSignTypedData(props: Props) {
  const { data, height, width } = props

  const [renderUnicode, setRenderUnicode] = React.useState<boolean>(true)

  return (
    <>
      {(hasUnicode(data?.messageJson ?? '') ||
        hasUnicode(data?.domainJson ?? '')) && (
        <WarningBox warningType='warning'>
          <WarningTitleRow>
            <WarningIcon color={'warningIcon'} />
            <WarningTitle warningType='warning'>
              {getLocale('braveWalletNonAsciiCharactersInMessageWarning')}
            </WarningTitle>
          </WarningTitleRow>
          <LearnMoreButton onClick={() => setRenderUnicode((prev) => !prev)}>
            {renderUnicode
              ? getLocale('braveWalletViewDecodedMessage')
              : getLocale('braveWalletViewEncodedMessage')}
          </LearnMoreButton>
        </WarningBox>
      )}

      {data && (
        <MessageBox
          height={height ?? '180px'}
          width={width}
        >
          <MessageHeader>
            {getLocale('braveWalletSignTransactionEIP712MessageDomain')}:
          </MessageHeader>
          <MessageText>
            {!renderUnicode && hasUnicode(data.domainJson)
              ? unicodeEscape(data.domainJson)
              : data.domainJson}
          </MessageText>

          <MessageHeader>
            {getLocale('braveWalletSignTransactionMessageTitle')}:
          </MessageHeader>
          <MessageText>
            {!renderUnicode && hasUnicode(data.messageJson)
              ? unicodeEscape(data.messageJson)
              : data.messageJson}
          </MessageText>
        </MessageBox>
      )}
    </>
  )
}
