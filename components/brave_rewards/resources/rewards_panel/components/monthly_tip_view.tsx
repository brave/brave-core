/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { PublisherInfo } from '../lib/interfaces'
import { TokenAmount } from '../../shared/components/token_amount'
import { CaretIcon } from '../../shared/components/icons/caret_icon'

import * as styles from './monthly_tip_view.style'

interface Props {
  publisherInfo: PublisherInfo
  onUpdateClick: () => void
  onCancelClick: () => void
}

export function MonthlyTipView (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [showActions, setShowActions] = React.useState(false)

  const { monthlyContribution } = props.publisherInfo

  function cancelLink (evt: React.UIEvent) {
    evt.preventDefault()
  }

  if (monthlyContribution <= 0) {
    return (
      <styles.root>
        <styles.setBox onClick={cancelLink}>
          <a href='#' onClick={props.onUpdateClick}>{getString('set')}</a>
        </styles.setBox>
      </styles.root>
    )
  }

  function toggleActionBubble () {
    setShowActions(!showActions)
  }

  return (
    <styles.root>
      <styles.amountBox>
        <styles.amount>
          <button onClick={toggleActionBubble}>
            <TokenAmount
              amount={monthlyContribution}
              minimumFractionDigits={0}
            /> <CaretIcon direction='down' />
          </button>
        </styles.amount>
        {
          showActions &&
            <styles.actionBubble onClick={cancelLink}>
              <styles.actionBubbleContent>
                <styles.action>
                  <a href='#' onClick={props.onUpdateClick}>
                    {getString('changeAmount')}
                  </a>
                </styles.action>
                <styles.action>
                  <a href='#' onClick={props.onCancelClick}>
                    {getString('cancel')}
                  </a>
                </styles.action>
                <styles.backdrop onClick={toggleActionBubble} />
              </styles.actionBubbleContent>
            </styles.actionBubble>
        }
      </styles.amountBox>
    </styles.root>
  )
}
