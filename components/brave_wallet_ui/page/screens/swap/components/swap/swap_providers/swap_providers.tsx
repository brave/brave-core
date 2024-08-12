// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Selectors
import {
  useSafeUISelector //
} from '../../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../../common/selectors'

// Constants
import { SwapProviderMetadata } from '../../../constants/metadata'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Types
import {
  BraveWallet,
  SupportedSwapProviders,
  SwapProviderNameMapping
} from '../../../../../../constants/types'

// Components
import {
  InfoIconTooltip //
} from '../../../../../../components/shared/info_icon_tooltip/info_icon_tooltip'

// Styles
import { RadioButton, ProviderIcon } from './swap_providers.style'
import {
  Column,
  HorizontalSpace,
  LeoSquaredButton,
  Row,
  Text
} from '../../../../../../components/shared/style'

interface Props {
  selectedProvider: BraveWallet.SwapProvider
  onChangeSwapProvider: (provider: BraveWallet.SwapProvider) => void
  availableProvidersForSwap: BraveWallet.SwapProvider[]
}

export const SwapProviders = (props: Props) => {
  const { selectedProvider, onChangeSwapProvider, availableProvidersForSwap } =
    props

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // State
  const [userSelectedSwapProvider, setUserSelectedSwapProvider] =
    React.useState<BraveWallet.SwapProvider>(selectedProvider)

  // Memos
  const providerList = React.useMemo(() => {
    return [...SupportedSwapProviders].sort(
      (a, b) =>
        Number(availableProvidersForSwap.includes(b)) -
        Number(availableProvidersForSwap.includes(a))
    )
  }, [availableProvidersForSwap])

  return (
    <Column fullWidth={true}>
      <Row
        marginBottom='14px'
        justifyContent={isPanel ? 'center' : 'flex-start'}
        padding='0px 16px'
      >
        <Text
          textSize={isPanel ? '16px' : '22px'}
          isBold={true}
          textColor='primary'
        >
          {getLocale('braveWalletChooseQuoteProvider')}
        </Text>
        <HorizontalSpace space='8px' />
        <InfoIconTooltip
          placement='bottom'
          text={getLocale('braveWalletQuoteProviderInfo')}
        />
      </Row>
      <Column
        fullWidth={true}
        padding='0px 16px'
        gap='8px'
      >
        {providerList.map((provider) => {
          // Computed
          const isAvailableForSwap =
            availableProvidersForSwap.includes(provider)
          return (
            <RadioButton
              onChange={() => setUserSelectedSwapProvider(provider)}
              name='providers'
              value={SwapProviderNameMapping[provider]}
              currentValue={SwapProviderNameMapping[userSelectedSwapProvider]}
              key={SwapProviderNameMapping[provider]}
              isSelected={userSelectedSwapProvider === provider}
              isDisabled={!isAvailableForSwap}
            >
              <Row
                justifyContent='space-between'
                padding='16px 0px'
              >
                <Row width='unset'>
                  <ProviderIcon src={SwapProviderMetadata[provider]} />
                  <Text
                    textSize='14px'
                    isBold={true}
                    textColor={!isAvailableForSwap ? 'disabled' : 'primary'}
                  >
                    {SwapProviderNameMapping[provider]}
                  </Text>
                </Row>
                {!isAvailableForSwap && (
                  <Text
                    textSize='14px'
                    isBold={false}
                    textColor='disabled'
                  >
                    {getLocale('braveWalletNotAvailable')}
                  </Text>
                )}
              </Row>
            </RadioButton>
          )
        })}
      </Column>
      <Row padding='16px'>
        <LeoSquaredButton
          onClick={() => onChangeSwapProvider(userSelectedSwapProvider)}
          size='large'
          isDisabled={userSelectedSwapProvider === selectedProvider}
        >
          {getLocale('braveWalletUpdate')}
        </LeoSquaredButton>
      </Row>
    </Column>
  )
}
