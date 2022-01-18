import * as React from 'react'

// Types
import {
  BraveWallet,
  DefaultCurrencies,
  WalletAccountType,
  AddAccountNavTypes
} from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import {
  formatFiatAmountWithCommasAndDecimals
} from '../../../../../../utils/format-prices'

// Components
import {
  PortfolioTransactionItem,
  PortfolioAccountItem,
  AddButton
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
  DividerRow,
  CoinGeckoText
} from '../../style'

export interface Props {
  transactionSpotPrices: BraveWallet.AssetPrice[]
  defaultCurrencies: DefaultCurrencies
  selectedAsset: BraveWallet.BlockchainToken | undefined
  accounts: WalletAccountType[]
  selectedNetwork: BraveWallet.EthereumChain
  fullAssetFiatBalance: string
  formattedFullAssetBalance: string
  selectedAssetTransactions: BraveWallet.TransactionInfo[]
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  onSelectAccount: (account: WalletAccountType) => void
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken | undefined) => () => void
  onRetryTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onSpeedupTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onCancelTransaction: (transaction: BraveWallet.TransactionInfo) => void
}

const AccountsAndTransactionsList = (props: Props) => {
  const {
    selectedNetwork,
    transactionSpotPrices,
    defaultCurrencies,
    selectedAsset,
    accounts,
    fullAssetFiatBalance,
    formattedFullAssetBalance,
    selectedAssetTransactions,
    userVisibleTokensInfo,
    onSelectAccount,
    onClickAddAccount,
    onSelectAsset,
    onCancelTransaction,
    onRetryTransaction,
    onSpeedupTransaction
  } = props

  const getBalance = useBalance(selectedNetwork)

  const findAccount = (address: string): WalletAccountType | undefined => {
    return accounts.find((account) => address === account.address)
  }

  const accountsList = React.useMemo(() => {
    if (selectedAsset?.isErc721) {
      return accounts.filter((account) => Number(account.balance) !== 0)
    }
    return accounts
  }, [selectedAsset, accounts])

  return (
    <>
      {selectedAsset &&
        <>
          <DividerRow>
            <DividerText>{selectedAsset?.isErc721 ? getLocale('braveWalletOwner') : getLocale('braveWalletAccounts')}</DividerText>
            {!selectedAsset?.isErc721 &&
              <AssetBalanceDisplay>{formatFiatAmountWithCommasAndDecimals(fullAssetFiatBalance, defaultCurrencies.fiat)} {formattedFullAssetBalance}</AssetBalanceDisplay>
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
              selectedNetwork={selectedNetwork}
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
                  defaultCurrencies={defaultCurrencies}
                  key={transaction.id}
                  selectedNetwork={selectedNetwork}
                  accounts={accounts}
                  transaction={transaction}
                  account={findAccount(transaction.fromAddress)}
                  transactionSpotPrices={transactionSpotPrices}
                  visibleTokens={userVisibleTokensInfo}
                  displayAccountName={true}
                  onSelectAccount={onSelectAccount}
                  onSelectAsset={onSelectAsset}
                  onRetryTransaction={onRetryTransaction}
                  onSpeedupTransaction={onSpeedupTransaction}
                  onCancelTransaction={onCancelTransaction}
                />
              )}
            </>
          ) : (
            <EmptyTransactionContainer>
              <TransactionPlaceholderText>{getLocale('braveWalletTransactionPlaceholder')}</TransactionPlaceholderText>
            </EmptyTransactionContainer>
          )}
          <CoinGeckoText>{getLocale('braveWalletPoweredByCoinGecko')}</CoinGeckoText>
        </>
      }
    </>
  )
}

export default AccountsAndTransactionsList
