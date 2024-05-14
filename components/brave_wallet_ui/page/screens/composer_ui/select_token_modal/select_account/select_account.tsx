// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../constants/types'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery
} from '../../../../../common/slices/api.slice'
import {
  TokenBalancesRegistry //
} from '../../../../../common/slices/entities/token-balance.entity'

// Utils
import Amount from '../../../../../utils/amount'
import { reduceAddress } from '../../../../../utils/reduce-address'
import {
  formatTokenBalanceWithSymbol,
  getBalance
} from '../../../../../utils/balance-utils'
import { getLocale } from '../../../../../../common/locale'

// Components
import {
  withPlaceholderIcon //
} from '../../../../../components/shared/create-placeholder-icon/index'
import { NftIcon } from '../../../../../components/shared/nft-icon/nft-icon'
import {
  CreateNetworkIcon //
} from '../../../../../components/shared/create-network-icon'
import {
  CreateAccountIcon //
} from '../../../../../components/shared/create-account-icon/create-account-icon'

// Styles
import {
  Column,
  ScrollableColumn,
  LeoSquaredButton,
  Row,
  Text
} from '../../../../../components/shared/style'
import {
  IconsWrapper,
  AssetIcon,
  NetworkIconWrapper,
  AccountButton
} from './select_account.style'

const getFiatBalance = (
  accountId: BraveWallet.AccountId,
  token: BraveWallet.BlockchainToken,
  defaultFiatCurrency: string,
  tokenBalancesRegistry?: TokenBalancesRegistry | null,
  spotPrice?: BraveWallet.AssetPrice
) => {
  return spotPrice
    ? new Amount(getBalance(accountId, token, tokenBalancesRegistry))
        .divideByDecimals(token.decimals)
        .times(spotPrice.price)
        .formatAsFiat(defaultFiatCurrency)
    : Amount.empty().formatAsFiat(defaultFiatCurrency)
}

interface Props {
  token: BraveWallet.BlockchainToken
  accounts: BraveWallet.AccountInfo[]
  tokenBalancesRegistry?: TokenBalancesRegistry | null
  spotPrice?: BraveWallet.AssetPrice
  onSelectAccount: (account: BraveWallet.AccountInfo) => void
}

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 0 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)
const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

export const SelectAccount = (props: Props) => {
  const { token, accounts, tokenBalancesRegistry, spotPrice, onSelectAccount } =
    props

  // Queries
  const { data: tokensNetwork } = useGetNetworkQuery(token)
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

  // State
  const [selectedAccount, setSelectedAccount] =
    React.useState<BraveWallet.AccountInfo>(accounts[0])

  return (
    <Column
      fullWidth={true}
      fullHeight={true}
      justifyContent='flex-start'
    >
      <Column margin='0px 0px 24px 0px'>
        <IconsWrapper margin='0px 0px 8px 0px'>
          {token.isErc721 || token.isNft ? (
            <NftIconWithPlaceholder asset={token} />
          ) : (
            <AssetIconWithPlaceholder asset={token} />
          )}
          <NetworkIconWrapper>
            <CreateNetworkIcon
              network={tokensNetwork}
              marginRight={0}
            />
          </NetworkIconWrapper>
        </IconsWrapper>
        <Text
          isBold={true}
          textSize='22px'
          textColor='primary'
        >
          {getLocale('braveWalletChooseAccount')}
        </Text>
      </Column>
      <ScrollableColumn
        fullWidth={true}
        maxHeight='240px'
        padding='16px'
        gap='8px'
      >
        {accounts.map((account) => (
          <AccountButton
            key={account.accountId.uniqueKey}
            isSelected={
              account.accountId.uniqueKey ===
              selectedAccount.accountId.uniqueKey
            }
            onClick={() => setSelectedAccount(account)}
          >
            <Row width='unset'>
              <CreateAccountIcon
                size='medium'
                account={account}
                marginRight={8}
              />
              <Column alignItems='flex-start'>
                <Text
                  textSize='14px'
                  textColor='primary'
                  isBold={false}
                  textAlign='left'
                >
                  {account.name}
                </Text>
                <Text
                  textSize='12px'
                  textColor='secondary'
                  isBold={false}
                  textAlign='left'
                >
                  {reduceAddress(account.address)}
                </Text>
              </Column>
            </Row>
            <Column alignItems='flex-end'>
              <Text
                textSize='14px'
                textColor='primary'
                isBold={true}
                textAlign='right'
              >
                {formatTokenBalanceWithSymbol(
                  getBalance(account.accountId, token, tokenBalancesRegistry),
                  token.decimals,
                  token.symbol
                )}
              </Text>
              <Text
                textSize='12px'
                textColor='secondary'
                isBold={false}
                textAlign='right'
              >
                {getFiatBalance(
                  account.accountId,
                  token,
                  defaultFiatCurrency,
                  tokenBalancesRegistry,
                  spotPrice
                )}
              </Text>
            </Column>
          </AccountButton>
        ))}
      </ScrollableColumn>
      <Row padding='16px'>
        <LeoSquaredButton
          onClick={() => onSelectAccount(selectedAccount)}
          size='large'
        >
          {getLocale('braveWalletButtonContinue')}
        </LeoSquaredButton>
      </Row>
    </Column>
  )
}
