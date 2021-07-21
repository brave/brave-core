/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getDaysUntilRewardsPayment } from '../../lib/pending_rewards'
import { LocaleContext, formatMessage } from '../../lib/locale_context'

import { TokenAmount } from '../token_amount'
import { MoneyBagIcon } from '../icons/money_bag_icon'

import * as styles from './pending_rewards_view.style'

interface Props {
  amount: number
  nextPaymentDate: Date
}

export function PendingRewardsView (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  const estimatedPendingDays = getDaysUntilRewardsPayment(props.nextPaymentDate)

  return (
    <styles.root>
      {
        props.amount > 0 && estimatedPendingDays &&
          <styles.pendingRewards>
            <MoneyBagIcon />
            {
              formatMessage(getString('walletPendingRewardsText'), [
                <styles.pendingAmount key='amount'>
                  <span className='plus'>+</span>
                  <TokenAmount
                    minimumFractionDigits={1}
                    amount={props.amount}
                  />
                </styles.pendingAmount>,
                estimatedPendingDays
              ])
            }
          </styles.pendingRewards>
        }
    </styles.root>
  )
}
