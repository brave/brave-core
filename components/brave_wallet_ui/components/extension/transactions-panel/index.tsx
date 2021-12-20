import * as React from 'react'

import {
  BraveWallet,
  WalletAccountType,
  DefaultCurrencies,
  AccountTransactions
} from '../../../constants/types'

// Styled Components
import {
  StyledWrapper
} from './style'

import { TransactionsListItem } from '../'
import { sortTransactionByDate } from '../../../utils/tx-utils'

export interface Props {
  selectedNetwork: BraveWallet.EthereumChain
  selectedAccount: WalletAccountType
  transactions: AccountTransactions
  accounts: WalletAccountType[]
  visibleTokens: BraveWallet.ERCToken[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  defaultCurrencies: DefaultCurrencies
  onSelectTransaction: (transaction: BraveWallet.TransactionInfo) => void
}

const TransactionsPanel = (props: Props) => {
  const {
    transactions,
    selectedNetwork,
    visibleTokens,
    transactionSpotPrices,
    accounts,
    defaultCurrencies,
    selectedAccount,
    onSelectTransaction
  } = props

  const findAccount = (address: string): WalletAccountType | undefined => {
    return accounts.find((account) => address === account.address)
  }

  const transactionList = React.useMemo(() => {
    if (selectedAccount?.address && transactions[selectedAccount.address]) {
      return sortTransactionByDate(transactions[selectedAccount.address], 'descending')
    } else {
      return []
    }
  }, [selectedAccount, transactions])

  return (
    <StyledWrapper>
      {transactionList.map((transaction: BraveWallet.TransactionInfo) =>
        <TransactionsListItem
          onSelectTransaction={onSelectTransaction}
          defaultCurrencies={defaultCurrencies}
          key={transaction.id}
          selectedNetwork={selectedNetwork}
          accounts={accounts}
          transaction={transaction}
          account={findAccount(transaction.fromAddress)}
          transactionSpotPrices={transactionSpotPrices}
          visibleTokens={visibleTokens}
        />
      )}
    </StyledWrapper>
  )
}

export default TransactionsPanel
