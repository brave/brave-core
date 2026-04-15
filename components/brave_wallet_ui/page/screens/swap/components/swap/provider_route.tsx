// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'
import usePromise from '$web-common/usePromise'

// Types
import {
  BraveWallet,
  SwapProviderNameMapping,
} from '../../../../../constants/types'

// Constants
import { SwapProviderMetadata } from '../../constants/metadata'

// Styled Components
import { LPIcon } from '../shared-swap.styles'
import { Row, Text } from '../../../../../components/shared/style'

interface Props {
  provider: BraveWallet.SwapProvider
  sourcesLength: number
  iconSize?: string
  textSize?: '12px' | '14px'
  textColor?: 'primary' | 'secondary'
}

export const ProviderRoute = (props: Props) => {
  const {
    provider,
    sourcesLength,
    iconSize = '16px',
    textSize = '12px',
    textColor = 'secondary',
  } = props

  const providerName = SwapProviderNameMapping[provider] ?? ''
  const providerIcon = SwapProviderMetadata[provider] ?? ''

  const { result: stepsLocale } = usePromise(
    async () =>
      PluralStringProxyImpl.getInstance().getPluralString(
        'braveWalletExchangeNamePlusSteps',
        sourcesLength,
      ),
    [sourcesLength],
  )

  return (
    <Row
      width='unset'
      gap='4px'
    >
      {providerIcon !== '' && (
        <LPIcon
          icon={providerIcon}
          size={iconSize}
        />
      )}
      <Text
        textSize={textSize}
        isBold={true}
        textColor={textColor}
      >
        {stepsLocale ? stepsLocale.replace('$1', providerName) : providerName}
      </Text>
    </Row>
  )
}
