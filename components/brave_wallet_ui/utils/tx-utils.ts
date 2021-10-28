import { TransactionInfo } from '../constants/types'

type Order = 'ascending' | 'descending'

export const sortTransactionByDate = (transactions: TransactionInfo[], order: Order = 'ascending') => {
  return [...transactions].sort(function (x: TransactionInfo, y: TransactionInfo) {
    return order === 'ascending'
      ? Number(x.createdTime.microseconds) - Number(y.createdTime.microseconds)
      : Number(y.createdTime.microseconds) - Number(x.createdTime.microseconds)
  })
}
