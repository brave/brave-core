// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import {
  withPlaceholderIcon //
} from '../../../../../../components/shared/create-placeholder-icon'
import {
  CreateNetworkIcon //
} from '../../../../../../components/shared/create-network-icon/index'

// Styled Components
import {
  Button,
  ButtonIcon,
  FuelTank,
  GasBubble,
  SelectTokenButtonStyleProps
} from './select-token-or-network.style'
import {
  Text,
  HorizontalSpacer,
  Row,
  HiddenResponsiveRow
} from '../../shared-swap.styles'
import {
  AssetIcon //
} from '../../../../composer_ui/shared_composer.style'

interface Props extends SelectTokenButtonStyleProps {
  onClick: () => void
  text: string | undefined
  disabled?: boolean
  networkFeeFiatValue?: string
  isHeader?: boolean
  asset?: BraveWallet.BlockchainToken
  network?: BraveWallet.NetworkInfo | null
  iconType: 'network' | 'asset'
}

export const SelectTokenOrNetworkButton = (props: Props) => {
  const {
    onClick,
    buttonType,
    buttonSize,
    text,
    disabled,
    hasBackground,
    hasShadow,
    networkFeeFiatValue,
    isHeader,
    asset,
    network,
    iconType
  } = props

  // Memos
  const needsMorePadding = React.useMemo((): boolean => {
    if (!text) {
      return true
    }
    return text.length > 3
  }, [text])

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, {
      size: buttonSize === 'small' || buttonSize === 'medium' ? 'small' : 'big',
      marginLeft: 0,
      marginRight: 8
    })
  }, [buttonSize])

  return (
    <Button
      onClick={onClick}
      buttonType={buttonType}
      moreRightPadding={needsMorePadding}
      buttonSize={buttonSize}
      disabled={disabled}
      hasBackground={hasBackground}
      hasShadow={hasShadow}
    >
      <Row>
        {iconType === 'network' ? (
          <CreateNetworkIcon
            network={network}
            marginRight={8}
            size={buttonSize === 'small' ? 'small' : 'big'}
          />
        ) : (
          text && <AssetIconWithPlaceholder asset={asset} />
        )}
        <HiddenResponsiveRow dontHide={!isHeader}>
          <Text
            isBold={text !== undefined}
            textColor={text ? 'text01' : 'text03'}
            textSize={
              buttonSize === 'small' || buttonSize === 'medium'
                ? '14px'
                : '18px'
            }
          >
            {text ?? getLocale('braveSwapSelectToken')}
          </Text>
        </HiddenResponsiveRow>
      </Row>
      <HiddenResponsiveRow dontHide={!isHeader}>
        {networkFeeFiatValue && (
          <>
            <HorizontalSpacer size={8} />
            <GasBubble>
              <FuelTank
                name='search-fuel-tank'
                size={16}
              />
              <Text
                textSize='14px'
                textColor='text01'
              >
                {networkFeeFiatValue}
              </Text>
            </GasBubble>
          </>
        )}
        {buttonSize !== 'small' && <HorizontalSpacer size={8} />}
      </HiddenResponsiveRow>
      <ButtonIcon
        size={24}
        name='carat-down'
      />
    </Button>
  )
}
