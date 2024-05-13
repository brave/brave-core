// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { getAssetIdKey } from '../../../utils/asset-utils'

// Components
import withPlaceholderIcon from '../../shared/create-placeholder-icon'

// Styled Components
import {
  StackContainer,
  AssetIcon,
  IconWrapper,
  AdditionalCountBubble
} from './icon-stacks.style'

// Methods
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, {
  size: 'tiny',
  marginLeft: 0,
  marginRight: 0
})

const calculateIconLeftPosition = (index: number) => {
  if (index === 0) {
    return 4
  }
  return index * 12 + 4
}

interface Props {
  tokens: BraveWallet.BlockchainToken[]
}

export const TokenIconsStack = (props: Props) => {
  const { tokens } = props

  // Memos / Computed
  const additionalTokensLength = tokens.length - 3

  const firstThreeTokens = React.useMemo(() => {
    return tokens.slice(0, 3)
  }, [tokens])

  const calculatedContainerWidth = React.useMemo(() => {
    if (tokens.length === 0) {
      return 0
    }
    const additionalWidth = tokens.length > 9 ? 16 : 12
    const firstThreeWidth = firstThreeTokens.length * 12 + additionalWidth
    if (tokens.length > 3) {
      return firstThreeWidth + additionalWidth
    }
    return firstThreeWidth
  }, [firstThreeTokens, tokens])

  return (
    <StackContainer width={`${calculatedContainerWidth}px`}>
      {firstThreeTokens.map((token, i) => (
        <IconWrapper
          key={getAssetIdKey(token)}
          leftPosition={calculateIconLeftPosition(i)}
        >
          <AssetIconWithPlaceholder asset={token} />
        </IconWrapper>
      ))}
      {tokens.length > 3 && (
        <IconWrapper leftPosition={calculateIconLeftPosition(3)}>
          <AdditionalCountBubble>
            {'+' + additionalTokensLength}
          </AdditionalCountBubble>
        </IconWrapper>
      )}
    </StackContainer>
  )
}
