// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

// Types / contants
import { BraveWallet } from '../../../constants/types'
import { UNKNOWN_TOKEN_COINGECKO_ID } from '../../../common/constants/magics'

// Utils
import { getLocale } from '../../../../common/locale'
import { findTokenByContractAddress } from '../../../utils/asset-utils'
import { getAccountLabel, getAddressLabel } from '../../../utils/account-utils'

// Styled components
import {
  NetworkText,
  StyledWrapper,
  TopRow,
  SignPanelButtonRow,
  HeaderTitle
} from './style'
import { WalletButton } from '../../shared/style'
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'

// Components
import { NavButton } from '../buttons/nav-button/index'
import {
  TransactionQueueSteps //
} from '../confirm-transaction-panel/common/queue'
import { Origin } from '../confirm-transaction-panel/common/origin'
import { SwapBase } from '../swap'
import { EthSignTypedData } from './common/eth_sign_typed_data'

// Queries
import {
  useGetCombinedTokensListQuery //
} from '../../../common/slices/api.slice.extra'
import {
  useGetAccountInfosRegistryQuery, //
  useGetNetworkQuery
} from '../../../common/slices/api.slice'

// Hooks
import { useAccountOrb, useAddressOrb } from '../../../common/hooks/use-orb'

const makeUnknownToken = (
  chainId: string,
  coin: BraveWallet.CoinType,
  contractAddress: string
) => ({
  chainId,
  coin,
  symbol: '???',
  contractAddress,
  logo: '',
  isErc721: false,
  isNft: false,
  name: '',
  coingeckoId: UNKNOWN_TOKEN_COINGECKO_ID,
  decimals: 0,
  isShielded: false
})

interface Props {
  data: BraveWallet.SignMessageRequest
  queueLength: number
  queueNumber: number
  isDisabled: boolean
  onQueueNextSignMessage: () => void
  onSignIn: () => void
  onCancel: () => void
}

export function SignCowSwapOrder(props: Props) {
  const {
    data,
    queueLength,
    queueNumber,
    isDisabled,
    onQueueNextSignMessage,
    onSignIn,
    onCancel
  } = props

  // State
  const [showDetails, setShowDetails] = React.useState<boolean>(false)

  const { data: combinedTokensList } = useGetCombinedTokensListQuery()
  const { data: accounts } = useGetAccountInfosRegistryQuery()

  const cowSwapOrder = data.signData.ethSignTypedData?.meta?.cowSwapOrder

  const buyToken = cowSwapOrder?.buyToken
    ? findTokenByContractAddress(cowSwapOrder.buyToken, combinedTokensList)
    : undefined
  const sellToken = cowSwapOrder?.sellToken
    ? findTokenByContractAddress(cowSwapOrder.sellToken, combinedTokensList)
    : undefined

  const { data: network } = useGetNetworkQuery({
    chainId: data.chainId,
    coin: data.coin
  })

  const senderLabel = accounts && getAccountLabel(data.accountId, accounts)
  const recipientLabel =
    accounts &&
    cowSwapOrder &&
    cowSwapOrder.receiver &&
    getAddressLabel(cowSwapOrder.receiver, accounts)
  const senderOrb = useAccountOrb({
    accountId: data.accountId,
    address: data.accountId.address
  })
  const recipientOrb = useAddressOrb(cowSwapOrder?.receiver, { scale: 10 })

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText />
        <TransactionQueueSteps
          queueNextTransaction={onQueueNextSignMessage}
          transactionQueueNumber={queueNumber}
          transactionsQueueLength={queueLength}
        />
      </TopRow>

      <HeaderTitle>{getLocale('braveWalletSwapReviewHeader')}</HeaderTitle>

      <Origin originInfo={data.originInfo} />

      {!showDetails && (
        <SwapBase
          sellToken={
            sellToken ||
            (cowSwapOrder
              ? makeUnknownToken(
                  data.chainId,
                  data.coin,
                  cowSwapOrder.sellToken
                )
              : undefined)
          }
          buyToken={
            buyToken ||
            (cowSwapOrder
              ? makeUnknownToken(data.chainId, data.coin, cowSwapOrder.buyToken)
              : undefined)
          }
          sellAmount={cowSwapOrder?.sellAmount}
          buyAmount={cowSwapOrder?.buyAmount}
          senderLabel={senderLabel}
          senderOrb={senderOrb}
          recipientOrb={recipientOrb}
          recipientLabel={recipientLabel}
          expectRecipientAddress={true}
        />
      )}

      {showDetails && (
        <EthSignTypedData
          data={data.signData.ethSignTypedData}
          height='220px'
          width='calc(100% - 8px)'
        />
      )}

      <NetworkFeeAndDetailsContainer>
        <NetworkFeeContainer>
          <NetworkFeeTitle>
            {getLocale('braveWalletNetworkFees')}
          </NetworkFeeTitle>
          <NetworkFeeValue>
            <CreateNetworkIcon
              network={network}
              marginRight={0}
            />
            {getLocale('braveSwapFree')}
          </NetworkFeeValue>
        </NetworkFeeContainer>
        <TextButton onClick={() => setShowDetails(!showDetails)}>
          {showDetails
            ? getLocale('braveWalletSignTransactionEIP712MessageHideDetails')
            : getLocale('braveWalletDetails')}
        </TextButton>
      </NetworkFeeAndDetailsContainer>

      <SignPanelButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletButtonCancel')}
          onSubmit={onCancel}
          disabled={isDisabled}
        />
        <NavButton
          buttonType='sign'
          text={getLocale('braveWalletSignTransactionButton')}
          onSubmit={onSignIn}
          disabled={isDisabled}
        />
      </SignPanelButtonRow>
    </StyledWrapper>
  )
}

const NetworkFeeAndDetailsContainer = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  width: calc(100% - 8px);
`

const NetworkFeeContainer = styled.div``

const NetworkFeeTitle = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 12px;
  line-height: 20px;
  color: ${(p) => p.theme.color.text03};
`

const NetworkFeeValue = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 18px;
  display: flex;
  align-items: center;
  letter-spacing: 0.01em;
  color: #27ae60;
  gap: 6px;
`

const TextButton = styled(WalletButton)`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0px;
  padding: 0px;
`
