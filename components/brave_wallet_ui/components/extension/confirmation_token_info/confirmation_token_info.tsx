// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Label from '@brave/leo/react/label'
import Icon from '@brave/leo/react/icon'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetTokenSpotPricesQuery,
} from '../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s, //
} from '../../../common/slices/constants'

// Utils
import { getLocale } from '../../../../common/locale'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { reduceAddress } from '../../../utils/reduce-address'
import {
  computeFiatAmount,
  getPriceIdForToken,
} from '../../../utils/pricing-utils'
import Amount from '../../../utils/amount'

// Types
import { BraveWallet } from '../../../constants/types'

// Components
import {
  withPlaceholderIcon, //
} from '../../shared/create-placeholder-icon/index'
import {
  CreateAccountIcon, //
} from '../../shared/create-account-icon/create-account-icon'
import { CreateNetworkIcon } from '../../shared/create-network-icon'

// Styled Components
import { Row, Column } from '../../shared/style'
import {
  AssetIcon,
  IconsWrapper,
  NetworkIconWrapper,
  TokenAmountText,
  AccountButton,
} from './confirmation_token_info.style'
import {
  ConfirmationInfoLabel,
  ConfirmationInfoText,
} from '../shared-panel-styles'

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 0 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)

interface Props {
  token?: BraveWallet.BlockchainToken
  network?: BraveWallet.NetworkInfo
  amount?: string
  account?: BraveWallet.AccountInfo
  receiveAddress?: string
  label: 'send' | 'spend' | 'receive' | 'bridge'
}

export function ConfirmationTokenInfo(props: Props) {
  const { token, label, amount, network, account, receiveAddress } = props

  // Queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    token
      && !token.isNft
      && !token.isErc721
      && !token.isErc1155
      && defaultFiatCurrency
      ? {
          ids: [getPriceIdForToken(token)],
          toCurrency: defaultFiatCurrency,
        }
      : skipToken,
    querySubscriptionOptions60s,
  )

  // Computed
  const labelText =
    label === 'send'
      ? getLocale('braveWalletSend')
      : label === 'spend'
        ? getLocale('braveWalletSpend')
        : getLocale('braveWalletReceive')

  return (
    <Row
      justifyContent='space-between'
      gap='16px'
      margin={label === 'bridge' ? '10px 0px 0px 0px' : '0px'}
    >
      {label === 'bridge' ? (
        <CreateNetworkIcon
          network={network}
          marginRight={0}
          size='extra-huge'
        />
      ) : (
        <IconsWrapper>
          <AssetIconWithPlaceholder asset={token} />
          <NetworkIconWrapper>
            {network && (
              <CreateNetworkIcon
                network={network}
                marginRight={0}
              />
            )}
          </NetworkIconWrapper>
        </IconsWrapper>
      )}
      <Column
        width='100%'
        alignItems='flex-start'
        justifyContent='flex-start'
        gap='4px'
      >
        <Row justifyContent='space-between'>
          <ConfirmationInfoLabel textColor='secondary'>
            {labelText}
          </ConfirmationInfoLabel>
          <AccountButton
            onClick={() => copyToClipboard(account?.address ?? '')}
          >
            <Label>
              {account ? (
                <Row>
                  <CreateAccountIcon
                    size='x-tiny'
                    account={account}
                    marginRight={4}
                  />
                  {account?.name ?? ''}
                </Row>
              ) : (
                reduceAddress(receiveAddress ?? '')
              )}
              <Icon
                name='copy'
                slot='icon-after'
              />
            </Label>
          </AccountButton>
        </Row>
        {label === 'bridge' && !token && (
          <TokenAmountText textColor='success'>
            {getLocale('braveWalletOnNetwork').replace(
              '$1',
              network?.chainName ?? '',
            )}
          </TokenAmountText>
        )}
        {label !== 'bridge' && amount && token && (
          <>
            <TokenAmountText
              textColor={label === 'receive' ? 'success' : 'primary'}
            >
              {new Amount(amount)
                .divideByDecimals(token.decimals)
                .formatAsAsset(6, token.symbol)}
            </TokenAmountText>
            <ConfirmationInfoText textColor='tertiary'>
              {computeFiatAmount({
                spotPriceRegistry,
                value: amount,
                token: token,
              }).formatAsFiat(defaultFiatCurrency)}
            </ConfirmationInfoText>
          </>
        )}
      </Column>
    </Row>
  )
}
