/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { GrantInfo, formatGrantMonth, formatGrantDaysToClaim } from '../../shared/lib/grant_info'
import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { TokenAmount } from '../../shared/components/token_amount'
import { MoneyBagIcon } from '../../shared/components/icons/money_bag_icon'

import * as style from './claim_grant_view.style'

interface Props {
  grantInfo: GrantInfo
  onClaim: () => void
}

export function ClaimGrantView (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const { grantInfo } = props

  const daysToClaim = formatGrantDaysToClaim(grantInfo)

  const title = grantInfo.type === 'ads'
    ? 'rewardsAdGrantTitle'
    : 'rewardsTokenGrantTitle'

  return (
    <style.root>
      <style.amount>
        <style.graphic><MoneyBagIcon /></style.graphic>
        {
          grantInfo.amount > 0 &&
            <div>
              <TokenAmount
                amount={grantInfo.amount}
                minimumFractionDigits={2}
              />
            </div>
        }
      </style.amount>
      <style.text>
        <style.title>
          {formatMessage(getString(title), [formatGrantMonth(grantInfo)])}
        </style.title>
        {
          daysToClaim &&
            <div>
              {
                formatMessage(getString('rewardsGrantDaysRemaining'), [
                  <style.days key='days'>{daysToClaim}</style.days>
                ])
              }
            </div>
        }
      </style.text>
      <style.action>
        <button onClick={props.onClaim}>
          {getString('claim')}
        </button>
      </style.action>
    </style.root>
  )
}
