/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { MoneyBagIcon } from '../../shared/components/icons/money_bag_icon'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'

import * as style from './arriving_soon_message.style'

interface Props {
  earningsLastMonth: number
  estimatedPendingDays: string
}

export function ArrivingSoonMessage (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  return (
    <style.root>
      <style.body>
        <MoneyBagIcon />
        {
          formatMessage(getString('pendingRewardsMessage'), [
            <span key='amount'>
              +{props.earningsLastMonth} BAT
            </span>,
            props.estimatedPendingDays
          ])
        }
      </style.body>
      <style.note>
        {
          formatMessage(getString('pendingRewardsNote'), {
            tags: {
              $1: (content) => <strong key='1'>{content}</strong>
            }
          })
        }
      </style.note>
    </style.root>
  )
}
