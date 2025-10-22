// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Label from '@brave/leo/react/label'
import Tooltip from '@brave/leo/react/tooltip'
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
  reduceAccountDisplayName, //
} from '../../../utils/reduce-account-name'
import {
  computeFiatAmount,
  getPriceRequestsForTokens,
} from '../../../utils/pricing-utils'
import {
  openAssociatedTokenAccountSupportArticleTab, //
} from '../../../utils/routes-utils'
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
  ArrowIconContainer,
  AddressText,
  WarningTooltip,
  WarningTooltipContent,
  LearnMoreButton,
  BlockExplorerButton,
} from './confirmation_token_info.style'
import {
  ConfirmationInfoLabel,
  ConfirmationInfoText,
} from '../shared-panel-styles'
import useExplorer from '../../../common/hooks/explorer'

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 0 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)

type TokenInfoLabel =
  | 'send'
  | 'spend'
  | 'receive'
  | 'bridge'
  | 'to'
  | 'shield'
  | 'fee'

interface Props {
  token?: BraveWallet.BlockchainToken
  network?: BraveWallet.NetworkInfo
  amount?: string
  valueExact?: string
  fiatValue?: string
  account?: BraveWallet.AccountInfo
  receiveAddress?: string
  isAssociatedTokenAccountCreation?: boolean
  label: TokenInfoLabel
}

export function ConfirmationTokenInfo(props: Props) {
  const {
    token,
    label,
    amount,
    network,
    account,
    receiveAddress,
    valueExact,
    fiatValue,
    isAssociatedTokenAccountCreation,
  } = props

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(network)

  // Queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const tokenPriceRequests = getPriceRequestsForTokens([token])
  const { data: spotPrices } = useGetTokenSpotPricesQuery(
    token
      && !token.isNft
      && !token.isErc721
      && !token.isErc1155
      && tokenPriceRequests.length
      && defaultFiatCurrency
      ? {
          requests: tokenPriceRequests,
          vsCurrency: defaultFiatCurrency,
        }
      : skipToken,
    querySubscriptionOptions60s,
  )

  // Computed
  const getLabelText = (label: TokenInfoLabel) => {
    switch (label) {
      case 'fee':
        return getLocale('braveWalletEstimatedFee')
      case 'send':
        return getLocale('braveWalletSend')
      case 'spend':
        return getLocale('braveWalletSpend')
      case 'shield':
        return getLocale('braveWalletShielding')
      default:
        return getLocale('braveWalletReceive')
    }
  }

  const ataCreationLocale = getLocale(
    'braveWalletConfirmTransactionAccountCreationFee',
  )

  // Memos
  const formattedAmount = React.useMemo(() => {
    if (valueExact) {
      return new Amount(valueExact).formatAsAsset(
        undefined,
        token?.symbol ?? '',
      )
    }
    if (amount && token) {
      return new Amount(amount)
        .divideByDecimals(token.decimals)
        .formatAsAsset(6, token.symbol)
    }
    return ''
  }, [valueExact, amount, token])

  const formattedFiatAmount = React.useMemo(() => {
    if (fiatValue) {
      return new Amount(fiatValue).formatAsFiat(defaultFiatCurrency)
    }
    if (amount && token) {
      return computeFiatAmount({
        spotPrices,
        value: amount,
        token: token,
      }).formatAsFiat(defaultFiatCurrency)
    }
    return ''
  }, [fiatValue, amount, token, defaultFiatCurrency, spotPrices])

  if (label === 'to' && account) {
    return (
      <Row
        justifyContent='space-between'
        gap='16px'
      >
        <CreateAccountIcon
          size='big'
          account={account}
        />
        <Column
          width='100%'
          alignItems='flex-start'
          justifyContent='flex-start'
        >
          <ConfirmationInfoLabel textColor='secondary'>
            {getLocale('braveWalletSwapTo')}
          </ConfirmationInfoLabel>
          <AddressText
            textColor='primary'
            textAlign='left'
          >
            {account.name}
          </AddressText>
          <Tooltip text={account.address}>
            <ConfirmationInfoText
              textColor='tertiary'
              textAlign='left'
            >
              {reduceAddress(account.address)}
            </ConfirmationInfoText>
          </Tooltip>
        </Column>
      </Row>
    )
  }

  if (label === 'to') {
    return (
      <Row
        justifyContent='space-between'
        gap='16px'
      >
        <ArrowIconContainer>
          <Icon name='arrow-right' />
        </ArrowIconContainer>
        <Row alignItems='flex-start'>
          <Column
            width='100%'
            alignItems='flex-start'
            justifyContent='flex-start'
            gap='4px'
          >
            <ConfirmationInfoLabel
              textColor='secondary'
              textAlign='left'
            >
              {getLocale('braveWalletSwapTo')}
            </ConfirmationInfoLabel>
            <Row gap='16px'>
              <Tooltip text={receiveAddress ?? ''}>
                <AddressText textColor='primary'>
                  {reduceAddress(receiveAddress ?? '')}
                </AddressText>
              </Tooltip>
              <BlockExplorerButton
                kind='plain-faint'
                fab
                onClick={onClickViewOnBlockExplorer(
                  'address',
                  receiveAddress ?? '',
                )}
              >
                <Icon name='launch' />
              </BlockExplorerButton>
            </Row>
          </Column>
          {isAssociatedTokenAccountCreation && (
            <WarningTooltip>
              <WarningTooltipContent
                slot='content'
                width='250px'
              >
                <span>
                  {ataCreationLocale}{' '}
                  <LearnMoreButton
                    onClick={openAssociatedTokenAccountSupportArticleTab}
                  >
                    {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
                  </LearnMoreButton>
                </span>
              </WarningTooltipContent>
              <Icon name='warning-circle-outline' />
            </WarningTooltip>
          )}
        </Row>
      </Row>
    )
  }

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
            {getLabelText(label)}
          </ConfirmationInfoLabel>
          <Tooltip
            text={account ? (account?.address ?? '') : (receiveAddress ?? '')}
          >
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
                    {reduceAccountDisplayName(account?.name ?? '', 18)}
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
          </Tooltip>
        </Row>
        {label === 'bridge' && !token && (
          <TokenAmountText
            textColor='success'
            textAlign='left'
          >
            {getLocale('braveWalletOnNetwork').replace(
              '$1',
              network?.chainName ?? '',
            )}
          </TokenAmountText>
        )}
        {label !== 'bridge' && formattedAmount && (
          <TokenAmountText
            textColor={label === 'receive' ? 'success' : 'primary'}
            textAlign='left'
          >
            {formattedAmount}
          </TokenAmountText>
        )}
        {label !== 'bridge' && formattedFiatAmount && (
          <ConfirmationInfoText
            textColor='tertiary'
            textAlign='left'
          >
            {formattedFiatAmount}
          </ConfirmationInfoText>
        )}
      </Column>
    </Row>
  )
}
