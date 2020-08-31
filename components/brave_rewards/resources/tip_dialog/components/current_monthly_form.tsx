/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext } from '../lib/host_context'
import { formatTokenAmount } from '../lib/formatting'

import { FormSubmitButton } from './form_submit_button'

import * as style from './current_monthly_form.style'

interface Props {
  currentMonthlyTip: number
  nextReconcileDate?: Date
  onCancelTip: () => void
  onChangeAmount: () => void
}

function getDateString (date?: Date) {
  if (!date) {
    return ''
  }
  return date.toLocaleDateString(undefined, {
    day: '2-digit',
    month: '2-digit',
    year: 'numeric'
  })
}

export function CurrentMonthlyForm (props: Props) {
  const host = React.useContext(HostContext)
  const { getString } = host

  const [confirmCancel, setConfirmCancel] = React.useState<boolean>(() => {
    return host.getDialogArgs().entryPoint === 'clear-monthly'
  })

  if (confirmCancel) {
    return (
      <style.root>
        <style.header>{getString('cancelMonthlyContribution')}</style.header>
        <style.cancelText>{getString('cancelConfirmationText')}</style.cancelText>
        <FormSubmitButton onClick={props.onCancelTip}>
          {getString('confirmCancel')}
        </FormSubmitButton>
      </style.root>
    )
  }

  function onCancelClick () {
    setConfirmCancel(true)
  }

  return (
    <style.root>
      <style.header>{getString('currentMonthlyContribution')}</style.header>
      <style.contributionTable>
        <table>
          <tbody>
            <tr>
              <td>{getString('contributionAmountLabel')}</td>
              <td>{formatTokenAmount(props.currentMonthlyTip)} {getString('bat')}</td>
            </tr>
            <tr>
              <td>{getString('nextContributionDate')}</td>
              <td>{getDateString(props.nextReconcileDate)}</td>
            </tr>
          </tbody>
        </table>
      </style.contributionTable>
      <style.buttons>
        <style.cancel>
          <button onClick={onCancelClick}>{getString('cancel')}</button>
        </style.cancel>
        <style.changeAmount>
          <button onClick={props.onChangeAmount}>{getString('changeAmount')}</button>
        </style.changeAmount>
      </style.buttons>
    </style.root>
  )
}
