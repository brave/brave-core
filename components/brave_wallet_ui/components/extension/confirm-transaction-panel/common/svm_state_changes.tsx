// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { color } from '@brave/leo/tokens/css/variables'
import Tooltip from '@brave/leo/react/tooltip'

// types & magics
import { BraveWallet } from '../../../../constants/types'
import type { ParsedTransaction } from '../../../../utils/tx-utils'

// utils
import Amount from '../../../../utils/amount'
import { getLocale } from '../../../../../common/locale'
import { reduceAddress } from '../../../../utils/reduce-address'
import { IconAsset } from '../../../shared/create-placeholder-icon'
import { getAddressLabel } from '../../../../utils/account-utils'
import {
  getNetworkId //
} from '../../../../common/slices/entities/network.entity'

// hooks
import {
  useGetCombinedTokensRegistryQuery,
  useGetIsRegistryTokenQuery
} from '../../../../common/slices/api.slice.extra'
import {
  useGetAccountInfosRegistryQuery //
} from '../../../../common/slices/api.slice'

// components
import { CopyTooltip } from '../../../shared/copy-tooltip/copy-tooltip'
import {
  ChainInfo,
  InlineViewOnBlockExplorerIconButton
} from './view_on_explorer_button'
import {
  AssetIconWithPlaceholder,
  NftAssetIconWithPlaceholder,
  NFT_ICON_STYLE
} from './state_change_asset_icons'
import { CopyLabel } from '../../../shared/copy_label/copy_label'

// style
import {
  Column,
  IconsWrapper,
  NetworkIconWrapper,
  Row,
  Text
} from '../../../shared/style'
import {
  ArrowRightIcon,
  UnverifiedTokenIndicator,
  StateChangeText,
  TooltipContent
} from './state_changes.styles'

type BlockchainInfo = Pick<
  BraveWallet.NetworkInfo,
  | 'blockExplorerUrls'
  | 'chainId'
  | 'chainName'
  | 'coin'
  | 'iconUrls'
  //
  | 'symbol'
>

const METAPLEX_NFT_KINDS = [
  BraveWallet.BlowfishMetaplexTokenStandardKind.kNonFungible,
  BraveWallet.BlowfishMetaplexTokenStandardKind.kNonFungibleEdition
] as const

export const SOLTransfer = ({
  network,
  transfer,
  transactionDetails
}: {
  transfer: BraveWallet.BlowfishSOLTransferData
  transactionDetails?: ParsedTransaction
  network: Pick<
    BraveWallet.NetworkInfo,
    'chainId' | 'symbol' | 'iconUrls' | 'chainName' | 'blockExplorerUrls'
  >
}): JSX.Element => {
  // memos
  const asset = React.useMemo(() => {
    return {
      contractAddress: '',
      isErc721: false,
      isNft: false,
      logo: network.iconUrls[0],
      name: transfer.asset.name,
      symbol: transfer.asset.symbol,
      chainId: network.chainId,
      isShielded: false
    }
  }, [transfer, network])

  const normalizedAmount = React.useMemo(() => {
    return new Amount(transfer.diff.digits).divideByDecimals(
      transfer.asset.decimals
    )
  }, [transfer])

  // computed
  const isReceive = transfer.diff.sign !== BraveWallet.BlowfishDiffSign.kMinus
  const isAssociatedTokenAccountCreation =
    transactionDetails?.isAssociatedTokenAccountCreation ?? false

  // render
  return (
    <Column
      alignItems='flex-start'
      padding={0}
      margin={'0px 0px 8px 0px'}
    >
      <Row
        alignItems='center'
        justifyContent='space-between'
      >
        <Text
          textSize='12px'
          color={color.text.secondary}
        >
          {getLocale(isReceive ? 'braveWalletReceive' : 'braveWalletSend')}
        </Text>

        {/**
         * counterparty not currently provided by Blowfish
         * show the account/address only if it is a simple transfer TX type
         */}
        {!isReceive &&
          (transactionDetails?.txType ===
            BraveWallet.TransactionType.SolanaSystemTransfer ||
            isAssociatedTokenAccountCreation) &&
          transactionDetails?.recipient && (
            <Tooltip>
              <TooltipContent slot='content'>
                {transactionDetails.recipient}
              </TooltipContent>
              <CopyLabel textToCopy={transactionDetails.recipient}>
                {getLocale('braveWalletSwapTo')}{' '}
                <strong>{transactionDetails.recipientLabel}</strong>
              </CopyLabel>
            </Tooltip>
          )}
      </Row>
      <Row
        margin={'4px 0px 0px 0px'}
        alignItems='center'
        justifyContent='flex-start'
        title={
          // treating all native assets as "verified"
          getLocale('braveWalletTokenIsVerified')
        }
      >
        <IconsWrapper marginRight='0px'>
          <AssetIconWithPlaceholder asset={asset} />
        </IconsWrapper>
        <Row
          alignItems='center'
          gap={'4px'}
          justifyContent='flex-start'
        >
          <StateChangeText
            color={isReceive ? color.systemfeedback.successIcon : undefined}
          >
            <strong>
              {isReceive ? ' + ' : ' - '}
              {normalizedAmount
                .toAbsoluteValue()
                .formatAsAsset(6, transfer.asset.symbol)}
            </strong>
          </StateChangeText>
        </Row>
      </Row>
    </Column>
  )
}

export const SPLTokenTransfer = ({
  network,
  transfer
}: {
  transfer: BraveWallet.BlowfishSPLTransferData
  network: BlockchainInfo
}): JSX.Element => {
  // queries
  const { isVerified } = useGetIsRegistryTokenQuery({
    address: transfer.asset.mint,
    chainId: network.chainId
  })
  const { data: accountsRegistry } = useGetAccountInfosRegistryQuery()

  const { data: combinedTokensRegistry } = useGetCombinedTokensRegistryQuery()

  // memos
  const asset: IconAsset = React.useMemo(() => {
    const foundTokenId = combinedTokensRegistry
      ? combinedTokensRegistry.idsByChainId[getNetworkId(network)].find(
          (id) =>
            combinedTokensRegistry.entities[id]?.contractAddress ===
            transfer.asset.mint
        )
      : undefined
    const foundToken = foundTokenId
      ? combinedTokensRegistry?.entities[foundTokenId]
      : undefined

    const assetLogo = foundToken?.logo || transfer.asset.imageUrl || ''

    return {
      contractAddress: transfer.asset.mint,
      isErc721: false,
      logo: assetLogo,
      name: transfer.asset.name,
      symbol: transfer.asset.symbol,
      chainId: network.chainId,
      isNft: METAPLEX_NFT_KINDS.includes(transfer.asset.metaplexTokenStandard),
      isShielded: false
    }
  }, [transfer, combinedTokensRegistry, network])

  const normalizedAmount = React.useMemo(() => {
    return new Amount(transfer.diff.digits).divideByDecimals(
      transfer.asset.decimals
    )
  }, [transfer])

  // computed
  const isReceive = transfer.diff.sign !== BraveWallet.BlowfishDiffSign.kMinus

  // render
  return (
    <Column
      alignItems='flex-start'
      padding={0}
      margin={'0px 0px 8px 0px'}
    >
      <Row
        alignItems='center'
        justifyContent='space-between'
      >
        <Text
          textSize='12px'
          color={color.text.secondary}
        >
          {getLocale(isReceive ? 'braveWalletReceive' : 'braveWalletSend')}
        </Text>
        {!isReceive && transfer.counterparty && (
          <Tooltip>
            <TooltipContent slot='content'>
              {transfer.counterparty}
            </TooltipContent>
            <CopyLabel textToCopy={transfer.counterparty}>
              {getLocale('braveWalletSwapTo')}{' '}
              <strong>
                {getAddressLabel(transfer.counterparty, accountsRegistry)}
              </strong>
            </CopyLabel>
          </Tooltip>
        )}
      </Row>
      <Row
        margin={'4px 0px 0px 0px'}
        alignItems='center'
        justifyContent='flex-start'
        title={
          isVerified
            ? getLocale('braveWalletTokenIsVerifiedByLists').replace('$1', '1')
            : getLocale('braveWalletTokenIsUnverified')
        }
      >
        <IconsWrapper marginRight='0px'>
          {asset.isNft ? (
            <NftAssetIconWithPlaceholder
              asset={asset}
              iconStyles={NFT_ICON_STYLE}
            />
          ) : (
            <AssetIconWithPlaceholder asset={asset} />
          )}
          {!isVerified && (
            <NetworkIconWrapper>
              <UnverifiedTokenIndicator />
            </NetworkIconWrapper>
          )}
        </IconsWrapper>

        <Row
          alignItems='center'
          gap={'4px'}
          justifyContent='flex-start'
        >
          <StateChangeText
            color={isReceive ? color.systemfeedback.successIcon : undefined}
          >
            <strong>
              {isReceive ? ' + ' : ' - '}
              {asset.isNft
                ? asset.name
                : normalizedAmount
                    .toAbsoluteValue()
                    .formatAsAsset(6, asset.symbol)}
            </strong>
          </StateChangeText>

          <InlineViewOnBlockExplorerIconButton
            address={asset.contractAddress}
            network={network}
            urlType={'contract'}
          />
        </Row>
      </Row>
    </Column>
  )
}

export const SPLTokenApproval = ({
  network,
  approval
}: {
  approval: BraveWallet.BlowfishSPLApprovalData
  network: BlockchainInfo
}): JSX.Element => {
  // computed
  const isNft =
    approval.asset.metaplexTokenStandard ===
      BraveWallet.BlowfishMetaplexTokenStandardKind.kNonFungible ||
    approval.asset.metaplexTokenStandard ===
      BraveWallet.BlowfishMetaplexTokenStandardKind.kNonFungibleEdition

  // memos
  const afterAmount = React.useMemo(() => {
    return new Amount(approval.diff.digits)
      .divideByDecimals(approval.asset.decimals)
      .formatAsAsset(6, approval.asset.symbol)
  }, [approval])

  // render
  return (
    <Column
      margin={'0px 0px 6px 0px'}
      alignItems='flex-start'
      justifyContent='center'
    >
      <Row
        gap={'4px'}
        alignItems='center'
        justifyContent='flex-start'
      >
        <StateChangeText>
          <strong>{isNft ? approval.asset.name : afterAmount}</strong>
        </StateChangeText>
      </Row>
      <Row
        alignItems='center'
        justifyContent='flex-start'
      >
        <CopyTooltip
          isAddress
          text={approval.delegate}
          tooltipText={approval.delegate}
          position='left'
          verticalPosition='below'
        >
          <Text textSize='11px'>
            {getLocale('braveWalletSpenderAddress').replace(
              '$1',
              reduceAddress(approval.delegate)
            )}
          </Text>
        </CopyTooltip>

        <InlineViewOnBlockExplorerIconButton
          address={approval.delegate}
          network={network}
          urlType={'address'}
        />
      </Row>
    </Column>
  )
}

const AddressChange = ({
  fromAddress,
  toAddress,
  network
}: {
  fromAddress: string
  toAddress: string
  network: ChainInfo
}) => {
  return (
    <Row
      alignItems='center'
      justifyContent='flex-start'
      padding={'8px 0px'}
    >
      <StateChangeText>
        <strong>
          {reduceAddress(fromAddress)}
          <InlineViewOnBlockExplorerIconButton
            address={fromAddress}
            network={network}
            urlType='address'
          />
        </strong>

        <ArrowRightIcon />

        <span>
          <strong>
            {reduceAddress(toAddress)}
            <InlineViewOnBlockExplorerIconButton
              address={toAddress}
              network={network}
              urlType='address'
            />
          </strong>
        </span>
      </StateChangeText>
    </Row>
  )
}

export const SolStakingAuthChange = ({
  authChange,
  network
}: {
  authChange: BraveWallet.BlowfishSOLStakeAuthorityChangeData
  network: BlockchainInfo
}) => {
  // computed from props
  const hasStakerChange =
    authChange.currentAuthorities.staker !== authChange.futureAuthorities.staker
  const hasWithdrawerChange =
    authChange.currentAuthorities.withdrawer !==
    authChange.futureAuthorities.withdrawer

  // render
  return (
    <Column
      margin={'0px 0px 6px 0px'}
      alignItems='flex-start'
      justifyContent='center'
    >
      {hasStakerChange ? (
        <Column
          justifyContent='center'
          alignItems='flex-start'
        >
          <Row
            alignItems='center'
            justifyContent='flex-start'
          >
            <StateChangeText>{getLocale('braveWalletStaker')}</StateChangeText>
          </Row>
          <AddressChange
            fromAddress={authChange.currentAuthorities.staker}
            toAddress={authChange.futureAuthorities.staker}
            network={network}
          />
        </Column>
      ) : null}

      {hasWithdrawerChange ? (
        <Column
          justifyContent='center'
          alignItems='flex-start'
        >
          <Row
            alignItems='center'
            justifyContent='flex-start'
          >
            <StateChangeText>
              {getLocale('braveWalletWithdrawer')}
            </StateChangeText>
          </Row>
          <AddressChange
            fromAddress={authChange.currentAuthorities.withdrawer}
            toAddress={authChange.futureAuthorities.withdrawer}
            network={network}
          />
        </Column>
      ) : null}
    </Column>
  )
}

export const SolAccountOwnershipChange = ({
  ownerChange,
  network
}: {
  ownerChange: BraveWallet.BlowfishSolanaUserAccountOwnerChangeData
  network: BlockchainInfo
}) => {
  // render
  return (
    <Column
      margin={'0px 0px 6px 0px'}
      alignItems='flex-start'
      justifyContent='center'
    >
      <Row
        alignItems='center'
        justifyContent='flex-start'
      >
        <StateChangeText>{getLocale('braveWalletOwner')}</StateChangeText>
      </Row>
      <AddressChange
        fromAddress={ownerChange.currentOwner}
        toAddress={ownerChange.futureOwner}
        network={network}
      />
    </Column>
  )
}

export function getComponentForSvmTransfer(
  transfer: BraveWallet.BlowfishSolanaStateChange,
  network: ChainInfo,
  transactionDetails?: ParsedTransaction
) {
  const { data } = transfer.rawInfo

  if (data.solTransferData) {
    return (
      <SOLTransfer
        key={transfer.humanReadableDiff}
        transfer={data.solTransferData}
        network={network}
        transactionDetails={transactionDetails}
      />
    )
  }

  if (data.splTransferData) {
    return (
      <SPLTokenTransfer
        key={transfer.humanReadableDiff}
        transfer={data.splTransferData}
        network={network}
      />
    )
  }

  return null
}
