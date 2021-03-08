/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export { BAPDeprecationAlert } from './bap_deprecation_alert'
export { BAPDeprecationPopup } from './bap_deprecation_popup'

const bapAlertBegins = Date.parse('2021-03-06T00:00:00Z')
const bapCutoffBegins = Date.parse('2021-03-13T00:00:00Z')

interface InteractionState {
  popupShown: number
  alertDismissed: number
}

const interactionStateKey = 'bapDeprecationState'

function loadInteractionState (): InteractionState {
  let data: any
  try {
    data = JSON.parse(localStorage.getItem(interactionStateKey) || '{}')
  } catch {
    // Ignore
  }
  data = data || {}
  return {
    popupShown: Number(data.popupShown || 0),
    alertDismissed: Number(data.alertDismissed || 0)
  }
}

function saveInteractionState (state: Partial<InteractionState>) {
  localStorage.setItem(interactionStateKey, JSON.stringify({
    ...loadInteractionState(),
    ...state
  }))
}

export function saveBAPPopupShown () {
  saveInteractionState({ popupShown: Date.now() })
}

export function saveBAPAlertDismissed () {
  saveInteractionState({ alertDismissed: Date.now() })
}

function daysInPast (time: number) {
  const span = Date.now() - time
  return span / 1000 / 60 / 60 / 24
}

export function shouldShowBAPAlert (onlyAnonWallet: boolean, balance: number) {
  // Do not show the modal if external wallets are supported or if
  // the user has zero balance
  if (!onlyAnonWallet || balance <= 0) {
    return false
  }

  // Do not show the modal before the alert start date
  if (Date.now() < bapAlertBegins) {
    return false
  }

  const { alertDismissed } = loadInteractionState()

  if (alertDismissed) {
    // Do not show the modal if the user has dismissed it within the last 3 days
    // or if they have dismissed it and we are now past the BAP cutoff date
    if (daysInPast(alertDismissed) < 3 || Date.now() >= bapCutoffBegins) {
      return false
    }
  }

  return true
}

export function shouldShowBAPPopup (onlyAnonWallet: boolean, balance: number) {
  // Do not show the popup if external wallets are supported or if
  // the user has zero balance
  if (!onlyAnonWallet || balance <= 0) {
    return false
  }

  // Do not show the popup after the alert start date
  if (Date.now() >= bapAlertBegins) {
    return false
  }

  const { popupShown } = loadInteractionState()

  // Do not show the popup if it has been shown within the last 3 days
  if (popupShown && daysInPast(popupShown) < 3) {
    return false
  }

  return true
}
