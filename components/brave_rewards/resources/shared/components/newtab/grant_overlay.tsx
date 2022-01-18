import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { GrantInfo, formatGrantDaysToClaim, formatGrantMonth } from '../../lib/grant_info'
import { GrantAvailableGraphic } from '../grant_available_graphic'
import { TokenAmount } from '../token_amount'

import * as style from './grant_overlay.style'

function getGrantMessages (grantInfo: GrantInfo) {
  if (grantInfo.type === 'ads') {
    return {
      title: 'rewardsAdGrantTitle',
      amount: 'rewardsAdGrantAmount',
      button: 'rewardsClaimRewards'
    }
  }

  return {
    title: 'rewardsTokenGrantTitle',
    amount: '',
    button: 'rewardsClaimTokens'
  }
}

interface Props {
  grantInfo: GrantInfo
  onClaim: () => void
}

export function GrantOverlay (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  const { grantInfo } = props
  if (!grantInfo) {
    return null
  }

  const messages = getGrantMessages(grantInfo)
  const grantMonth = formatGrantMonth(grantInfo)
  const daysToClaim = formatGrantDaysToClaim(grantInfo)

  return (
    <style.root>
      <GrantAvailableGraphic />
      <style.title>
        {formatMessage(getString(messages.title), [grantMonth])}
      </style.title>
      {
        grantInfo.amount > 0 && messages.amount &&
          <style.amount>
            {
              formatMessage(getString(messages.amount), [
                grantMonth,
                <TokenAmount
                  key='amount'
                  amount={grantInfo.amount}
                  minimumFractionDigits={1}
                />
              ])
            }
          </style.amount>
      }
      <style.action>
        <button onClick={props.onClaim}>
          {getString(messages.button)}
        </button>
      </style.action>
      {
        daysToClaim &&
          <style.text>
            {
              formatMessage(getString('rewardsGrantDaysRemaining'), [
                <style.days key='days'>{daysToClaim}</style.days>
              ])
            }
          </style.text>
      }
    </style.root>
  )
}
