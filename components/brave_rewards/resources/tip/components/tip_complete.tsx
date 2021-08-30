/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { TwitterColorIcon } from 'brave-ui/components/icons'

import { TipKind } from '../lib/interfaces'
import { HostContext } from '../lib/host_context'
import { formatMessage, LocaleContext } from '../../shared/lib/locale_context'

import { TokenAmount } from '../../shared/components/token_amount'

import * as style from './tip_complete.style'

function getNextContribution (reconcileDate: Date | undefined) {
  if (!reconcileDate) {
    return ''
  }
  return reconcileDate.toLocaleDateString(undefined, {
    day: '2-digit',
    month: '2-digit',
    year: 'numeric'
  })
}

interface Props {
  tipKind: TipKind
  tipAmount: number
}

export function TipComplete (props: Props) {
  const host = React.useContext(HostContext)
  const { getString } = React.useContext(LocaleContext)

  const [nextContribution, setNextContribution] = React.useState(
    getNextContribution(host.state.nextReconcileDate))

  React.useEffect(() => {
    return host.addListener((state) => {
      setNextContribution(getNextContribution(state.nextReconcileDate))
    })
  }, [host])

  function onShareClick () {
    host.shareTip('twitter')
  }

  function getSummaryTable () {
    if (props.tipKind === 'monthly') {
      return (
        <table>
          <tbody>
            <tr>
              <td>{getString('contributionAmount')}</td>
              <td><TokenAmount amount={props.tipAmount} /></td>
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
            <td><TokenAmount amount={props.tipAmount} /></td>
          </tr>
        </tbody>
      </table>
    )
  }

  if (props.tipAmount === 0 && props.tipKind === 'monthly') {
    return (
      <style.root>
        <style.main>
          <style.cancelHeader>
            {getString('sorryToSeeYouGo')}
          </style.cancelHeader>
          <style.cancelText>
            {getString('contributionCanceled')}
          </style.cancelText>
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
          {
            props.tipKind === 'one-time' &&
              <style.delayNote>
                {
                  formatMessage(getString('tipDelayNote'), {
                    tags: {
                      $1: (content) => <strong key='label'>{content}</strong>
                    }
                  })
                }
              </style.delayNote>
          }
        </style.main>
        <style.share>
          <button onClick={onShareClick}>
            <style.shareIcon>
              <TwitterColorIcon />
            </style.shareIcon>
            {getString('tweetAboutSupport')}
          </button>
        </style.share>
      </style.root>
    </style.success>
  )
}
