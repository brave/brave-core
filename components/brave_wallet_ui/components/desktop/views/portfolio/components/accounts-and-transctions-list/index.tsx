import * as React from 'react'
import { useSelector } from 'react-redux'

// Types
import {
  BraveWallet,
  WalletAccountType,
  AddAccountNavTypes,
  WalletState
} from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import Amount from '../../../../../../utils/amount'
import { getTokensCoinType, getTokensNetwork } from '../../../../../../utils/network-utils'

// Components
import {
  PortfolioTransactionItem,
  PortfolioAccountItem,
  AddButton,
  WithHideBalancePlaceholder
} from '../../../../'

// Hooks
import { useBalance } from '../../../../../../common/hooks'

// Styled Components
import {
  ButtonRow,
  DividerText,
  SubDivider,
  EmptyTransactionContainer,
  TransactionPlaceholderText,
  AssetBalanceDisplay,
  DividerRow
} from '../../style'

export interface Props {
  selectedAsset: BraveWallet.BlockchainToken | undefined
  networkList: BraveWallet.NetworkInfo[]
  fullAssetFiatBalance: Amount
  formattedFullAssetBalance: string
  selectedAssetTransactions: BraveWallet.TransactionInfo[]
  hideBalances: boolean
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
}

const AccountsAndTransactionsList = (props: Props) => {
  const {
    selectedAsset,
    fullAssetFiatBalance,
    formattedFullAssetBalance,
    selectedAssetTransactions,
    hideBalances,
    networkList,
    onClickAddAccount
  } = props

  // redux
  const {
    transactionSpotPrices,
    accounts,
    defaultCurrencies,
    selectedNetwork
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const selectedAssetsNetwork = React.useMemo(() => {
    if (!selectedAsset) {
      return selectedNetwork
    }
    return getTokensNetwork(networkList, selectedAsset)
  }, [selectedNetwork, selectedAsset, networkList])

  const filteredAccountsByCoinType = React.useMemo(() => {
    if (!selectedAsset) {
      return []
    }
    const coinType = getTokensCoinType(networkList, selectedAsset)
    return accounts.filter((account) => account.coin === coinType)
  }, [networkList, accounts, selectedAsset])

  const getBalance = useBalance(networkList)

  const findAccount = React.useCallback((address: string): WalletAccountType | undefined => {
    return filteredAccountsByCoinType.find((account) => address === account.address)
  }, [filteredAccountsByCoinType])

  const accountsList = React.useMemo(() => {
    if (selectedAsset?.isErc721) {
      return filteredAccountsByCoinType.filter((account) => Number(account.nativeBalanceRegistry[selectedAssetsNetwork.chainId] ?? 0) !== 0)
    }
    return filteredAccountsByCoinType
  }, [selectedAsset, filteredAccountsByCoinType])

  return (
    <>
      {selectedAsset &&
        <>
          <DividerRow>
            <DividerText>{selectedAsset?.isErc721 ? getLocale('braveWalletOwner') : getLocale('braveWalletAccounts')}</DividerText>
            {!selectedAsset?.isErc721 &&
              <WithHideBalancePlaceholder
                size='small'
                hideBalances={hideBalances}
              >
                <AssetBalanceDisplay>
                  {fullAssetFiatBalance.formatAsFiat(defaultCurrencies.fiat)} {formattedFullAssetBalance}
                </AssetBalanceDisplay>
              </WithHideBalancePlaceholder>
            }
          </DividerRow>
          <SubDivider />
          {accountsList.map((account) =>
            <PortfolioAccountItem
              spotPrices={transactionSpotPrices}
              defaultCurrencies={defaultCurrencies}
              key={account.address}
              assetTicker={selectedAsset.symbol}
              assetDecimals={selectedAsset.decimals}
              name={account.name}
              address={account.address}
              assetBalance={getBalance(account, selectedAsset)}
              selectedNetwork={selectedAssetsNetwork}
              hideBalances={hideBalances}
            />
          )}
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={onClickAddAccount('create')}
              text={getLocale('braveWalletAddAccount')}
            />
          </ButtonRow>
          <DividerText>{getLocale('braveWalletTransactions')}</DividerText>
          <SubDivider />
          {selectedAssetTransactions.length !== 0 ? (
            <>
              {selectedAssetTransactions.map((transaction: BraveWallet.TransactionInfo) =>
                <PortfolioTransactionItem
                  key={transaction.id}
                  accounts={filteredAccountsByCoinType}
                  transaction={transaction}
                  account={findAccount(transaction.fromAddress)}
                  displayAccountName={true}
                />
              )}
            </>
          ) : (
            <EmptyTransactionContainer>
              <TransactionPlaceholderText>{getLocale('braveWalletTransactionPlaceholder')}</TransactionPlaceholderText>
            </EmptyTransactionContainer>
          )}
        </>
      }
    </>
  )
}

export default AccountsAndTransactionsList
