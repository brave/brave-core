/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { TipKind } from '../lib/interfaces'
import { HostContext } from '../lib/host_context'

import { BatString } from './bat_string'
import { TwitterIcon } from './icons/twitter_icon'

import * as style from './tip_complete.style'

interface Props {
  tipKind: TipKind
  tipAmount: number
}

export function TipComplete (props: Props) {
  const host = React.useContext(HostContext)
  const { getString } = host

  const [nextContribution, setNextContribution] = React.useState<string>('')

  React.useEffect(() => {
    return host.addListener((state) => {
      if (state.nextReconcileDate) {
        const dateString = state.nextReconcileDate.toLocaleDateString(undefined, {
          day: '2-digit',
          month: '2-digit',
          year: 'numeric'
        })
        setNextContribution(dateString)
      }
    })
  }, [host])

  const amountString = props.tipAmount.toFixed(
    Math.floor(props.tipAmount) === props.tipAmount ? 0 : 3
  )

  function onShareClick () {
    host.shareTip('twitter')
  }

  function getSummaryTable () {
    if (props.tipKind === 'monthly') {
      return (
        <table>
          <tbody>
            <tr>
              <td>{getString('contributionAmountLabel')}</td>
              <td><strong>{amountString}</strong> <BatString /></td>
            </tr>
            <tr>
              <td>{getString('nextContributionDate')}</td>
              <td>{nextContribution}</td>
            </tr>
          </tbody>
        </table>
      )
    }
    return (
      <table>
        <tbody>
          <tr>
            <td>{getString('oneTimeTipAmount')}</td>
            <td><strong>{amountString}</strong> <BatString /></td>
          </tr>
        </tbody>
      </table>
    )
  }

  if (props.tipAmount === 0 && props.tipKind === 'monthly') {
    return (
      <style.root>
        <style.main>
          <style.cancelHeader>{getString('sorryToSeeYouGo')}</style.cancelHeader>
          <style.cancelText>{getString('contributionCanceled')}</style.cancelText>
        </style.main>
      </style.root>
    )
  }

  return (
    <style.success>
      <style.root>
        <style.main>
          <style.header>
            {getString('thanksForTheSupport')}
          </style.header>
          <style.message>
            {
              getString(props.tipKind === 'monthly'
                ? 'monthlyContributionSet'
                : 'tipHasBeenSent')
            }
          </style.message>
          <style.table>
            {getSummaryTable()}
          </style.table>
        </style.main>
        <style.share>
          <button onClick={onShareClick}>
            <TwitterIcon />
            {getString('tweetAboutSupport')}
          </button>
        </style.share>
      </style.root>
    </style.success>
  )
}
