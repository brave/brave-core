/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  reconcile: RewardsInternals.CurrentReconcile
}

// Utils
import { getLocale } from '../../../../common/locale'

const getRetryStepString = (retryStep: number) => {
  switch (retryStep) {
    case 1:
      return getLocale('retryStepReconcile')
    case 2:
      return getLocale('retryStepCurrent')
    case 3:
      return getLocale('retryStepPayload')
    case 4:
      return getLocale('retryStepRegister')
    case 5:
      return getLocale('retryStepViewing')
    case 6:
      return getLocale('retryStepWinners')
    case 7:
      return getLocale('retryStepPrepare')
    case 8:
      return getLocale('retryStepProof')
    case 9:
      return getLocale('retryStepVote')
    case 10:
      return getLocale('retryStepFinal')
  }

  return getLocale('retryStepUnknown')
}

export const Contribution = (props: Props) => (
  <>
    {getLocale('viewingId')} {props.reconcile.viewingId || ''}
    <br/>
    {getLocale('amount')} {props.reconcile.amount || ''}
    <br/>
    {getLocale('retryStep')} {getRetryStepString(props.reconcile.retryStep) || ''}
    <br/>
    {getLocale('retryLevel')} {props.reconcile.retryLevel || ''}
    <br/>
  </>
)
