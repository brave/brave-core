// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'
import { color } from '@brave/leo/tokens/css'

// magics
import {
  BLOWFISH_UNLIMITED_VALUE,
  NATIVE_EVM_ASSET_CONTRACT_ADDRESS
} from '../../../../common/constants/magics'

// types
import { BraveWallet } from '../../../../constants/types'

// utils
import Amount from '../../../../utils/amount'
import { getLocale } from '../../../../../common/locale'
import { reduceAddress } from '../../../../utils/reduce-address'
import { IconAsset } from '../../../shared/create-placeholder-icon'
import {
  useGetCombinedTokensRegistryQuery //
} from '../../../../common/slices/api.slice.extra'

// components
import { CopyTooltip } from '../../../shared/copy-tooltip/copy-tooltip'
import {
  AssetIconWithPlaceholder,
  NFT_ICON_STYLE,
  NftAssetIconWithPlaceholder
} from './state_change_asset_icons'
import {
  ChainInfo,
  InlineViewOnBlockExplorerIconButton
} from './view_on_explorer_button'

// style
import {
  Column,
  IconsWrapper,
  NetworkIconWrapper,
  Row,
  Text
} from '../../../shared/style'
import {
  UnverifiedTokenIndicator,
  StateChangeText,
  ArrowRightIcon
} from './state_changes.styles'

type EVMApprovalData =
  | BraveWallet.BlowfishERC20ApprovalData
  | BraveWallet.BlowfishERC721ApprovalData
  | BraveWallet.BlowfishERC721ApprovalForAllData
  | BraveWallet.BlowfishERC1155ApprovalForAllData

export const EvmNativeAssetOrErc20TokenTransfer = ({
  network,
  transfer
}: {
  transfer:
    | BraveWallet.BlowfishERC20TransferData
    | BraveWallet.BlowfishNativeAssetTransferData
  network: ChainInfo
}): JSX.Element => {
  // queries
  const { data: tokensRegistry } = useGetCombinedTokensRegistryQuery(
    transfer.asset.imageUrl ? skipToken : undefined
  )

  // memos
  const asset: IconAsset = React.useMemo(() => {
    const foundTokenId = transfer.asset.imageUrl
      ? tokensRegistry.fungibleIdsByChainId[network.chainId].find(
          (id) =>
            tokensRegistry.entities[id]?.contractAddress?.toLowerCase() ===
            transfer.asset.address
        )
      : undefined

    const foundTokenLogo =
      foundTokenId !== undefined
        ? tokensRegistry.entities[foundTokenId]?.logo || ''
        : ''

    return {
      contractAddress: transfer.asset.address,
      isErc721: false,
      isNft: false,
      chainId: network.chainId,
      logo: transfer.asset.imageUrl || foundTokenLogo,
      name: transfer.asset.name,
      symbol: transfer.asset.symbol
    }
  }, [transfer.asset, network.chainId, tokensRegistry])

  // computed
  const normalizedAmount = new Amount(transfer.amount.after)
    .minus(transfer.amount.before)
    .divideByDecimals(transfer.asset.decimals)

  const isReceive = normalizedAmount.isPositive()

  const isNativeAsset =
    asset.contractAddress === NATIVE_EVM_ASSET_CONTRACT_ADDRESS ||
    asset.contractAddress === ''

  const getTokenVerificationString =
    getTransferTokenVerificationStatus(transfer)

  // render
  return (
    <Column
      alignItems='flex-start'
      padding={0}
      margin={'0px 0px 8px 0px'}
    >
      <Text
        textSize='12px'
        color={color.text.secondary}
      >
        {getLocale(isReceive ? 'braveWalletReceive' : 'braveWalletSend')}
      </Text>
      <Row
        margin={'4px 0px 0px 0px'}
        alignItems='center'
        justifyContent='flex-start'
      >
        <IconsWrapper
          marginRight='0px'
          title={getTokenVerificationString}
        >
          <AssetIconWithPlaceholder
            asset={asset}
            network={network}
          />
          {!transfer.asset.verified && (
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
              {normalizedAmount
                .toAbsoluteValue()
                .formatAsAsset(6, transfer.asset.symbol)}
            </strong>
          </StateChangeText>
          {!isNativeAsset && (
            <InlineViewOnBlockExplorerIconButton
              address={asset.contractAddress}
              network={network}
              urlType='token'
            />
          )}
        </Row>
      </Row>
    </Column>
  )
}

/**
 * For ERC721 & ERC1155 Transfers
 */
export const NonFungibleErcTokenTransfer = ({
  network,
  transfer
}: {
  network: ChainInfo
  transfer: Pick<
    | BraveWallet.BlowfishERC721TransferData
    | BraveWallet.BlowfishERC1155TransferData,
    'amount' | 'contract' | 'metadata' | 'name' | 'tokenId'
  >
}): JSX.Element => {
  // memos
  const asset: IconAsset = React.useMemo(() => {
    return {
      chainId: network.chainId,
      contractAddress: transfer.contract.address,
      isErc721: !!transfer.tokenId,
      isNft: !!transfer.tokenId,
      logo: transfer.metadata.rawImageUrl || '',
      name: transfer.name,
      symbol: transfer.name,
      tokenId: transfer.tokenId || ''
    }
  }, [transfer, network.chainId])

  // computed
  const isReceive = new Amount(transfer.amount.after) //
    .gt(transfer.amount.before)

  const tokenIdNumber = transfer.tokenId
    ? new Amount(transfer.tokenId).toNumber()
    : undefined

  // render
  return (
    <Column
      alignItems='flex-start'
      padding={0}
      margin={'0px 0px 8px 0px'}
    >
      <Text
        textSize='12px'
        color={color.text.secondary}
      >
        {getLocale(isReceive ? 'braveWalletReceive' : 'braveWalletSend')}
      </Text>
      <Row
        margin={'4px 0px 0px 0px'}
        alignItems='center'
        justifyContent='flex-start'
      >
        <IconsWrapper marginRight='0px'>
          {asset.isNft ? (
            <NftAssetIconWithPlaceholder
              asset={asset}
              network={network}
              iconStyles={NFT_ICON_STYLE}
            />
          ) : (
            <AssetIconWithPlaceholder
              asset={asset}
              network={network}
            />
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

              {asset.isNft &&
                `${asset.name}${
                  // show token id if not included in asset name
                  tokenIdNumber !== undefined &&
                  !asset.name.includes(tokenIdNumber.toString())
                    ? ` #${tokenIdNumber}`
                    : ''
                }`}

              {!asset.isNft &&
                new Amount(transfer.amount.after)
                  .minus(transfer.amount.before)
                  .toAbsoluteValue()
                  .formatAsAsset(6, asset.name || asset.symbol || '???')}
            </strong>
          </StateChangeText>

          <InlineViewOnBlockExplorerIconButton
            address={asset.contractAddress}
            id={transfer.tokenId}
            network={network}
            urlType={asset.isNft ? 'nft' : 'token'}
          />
        </Row>
      </Row>
    </Column>
  )
}

/**
 * Works for ERC20, ERC721, ERC1155
 */
export const ErcTokenApproval = ({
  approval,
  isERC20,
  network,
  isApprovalForAll
}: {
  network: ChainInfo
} & (
  | {
      isERC20?: false
      isApprovalForAll?: boolean
      approval: Pick<EVMApprovalData, 'amount' | 'contract' | 'spender'>
    }
  | {
      isERC20: true
      approval: BraveWallet.BlowfishERC20ApprovalData
      isApprovalForAll?: false
    }
)): JSX.Element => {
  /**
   * Not available for `BraveWallet.BlowfishERC1155ApprovalForAllData`
   */
  const assetSymbol = isERC20
    ? approval?.asset?.symbol || undefined
    : (
        approval as
          | BraveWallet.BlowfishERC721ApprovalForAllData
          | BraveWallet.BlowfishERC721ApprovalData
      )?.symbol ?? undefined

  // memos
  const beforeAmount = React.useMemo(() => {
    const before = new Amount(approval.amount.before)

    if (
      before.gte(BLOWFISH_UNLIMITED_VALUE) ||
      (before.gt(0) && isApprovalForAll)
    ) {
      return getLocale('braveWalletUnlimitedAssetAmount').replace(
        '$1',
        assetSymbol || ''
      )
    }

    if (isERC20) {
      return before
        .divideByDecimals(approval.asset.decimals)
        .formatAsAsset(6, assetSymbol)
    }
    return before.formatAsAsset(6, assetSymbol)
  }, [approval, isERC20])

  const afterAmount = React.useMemo(() => {
    const after = new Amount(approval.amount.after)

    if (
      after.gte(BLOWFISH_UNLIMITED_VALUE) ||
      (after.gt(0) && isApprovalForAll)
    ) {
      return getLocale('braveWalletUnlimitedAssetAmount').replace(
        '$1',
        assetSymbol || ''
      )
    }

    if (isERC20) {
      return after
        .divideByDecimals(approval.asset.decimals)
        .formatAsAsset(6, assetSymbol)
    }
    return after.formatAsAsset(6, assetSymbol)
  }, [approval, isERC20, assetSymbol, isApprovalForAll])

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
          <span>{getLocale('braveWalletFrom')}</span>
          <strong>{beforeAmount}</strong>
          <ArrowRightIcon />
          <span>{getLocale('braveWalletSwapTo')}</span>
          <strong>{afterAmount}</strong>
        </StateChangeText>
      </Row>
      <Row
        alignItems='center'
        justifyContent='flex-start'
      >
        <CopyTooltip
          isAddress
          text={approval.spender.address}
          tooltipText={approval.spender.address}
          position='left'
          verticalPosition='below'
        >
          <Text textSize='11px'>
            {getLocale('braveWalletSpenderAddress').replace(
              '$1',
              reduceAddress(approval.spender.address)
            )}
          </Text>
        </CopyTooltip>

        <InlineViewOnBlockExplorerIconButton
          address={approval.spender.address}
          network={network}
          urlType={'address'}
        />
      </Row>
    </Column>
  )
}

function getTransferTokenVerificationStatus(
  transfer:
    | BraveWallet.BlowfishERC20TransferData
    | BraveWallet.BlowfishNativeAssetTransferData
) {
  if (!transfer.asset.verified) {
    return getLocale('braveWalletTokenIsUnverified')
  }
  return transfer.asset.lists.length > 1
    ? getLocale('braveWalletTokenIsVerifiedByLists').replace(
        '$1',
        transfer.asset.lists.length.toString()
      )
    : getLocale('braveWalletTokenIsVerified')
}
