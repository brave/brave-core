/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../../../common/locale'

import { WithThemeVariables } from '../../../../brave_rewards/resources/shared/components/with_theme_variables'
import { LocaleContext } from '../../../../brave_rewards/resources/shared/lib/locale_context'
import {
  BAPDeprecationAlert,
  shouldShowBAPAlert,
  shouldShowBAPPopup,
  saveBAPAlertDismissed,
  saveBAPPopupShown
} from '../../../../brave_rewards/resources/shared/components/bap_deprecation'

const locale = {
  getString: getLocale
}

function modalRequestInHash () {
  return /^#?bap-deprecation$/i.test(window.location.hash)
}

interface Props {
  rewardsState: NewTab.RewardsWidgetState
}

// A modal that will display a BAP deprecation notice for Rewards users
// in Japan. This component is temporary and should be removed in 1.23.x
export default function BAPDeprecationModal (props: Props) {
  const { onlyAnonWallet, balance } = props.rewardsState

  const [popupShown, setPopupShown] = React.useState(false)

  const [showModal, setShowModal] = React.useState(
    modalRequestInHash() ||
    shouldShowBAPAlert(onlyAnonWallet, balance.total))

  React.useEffect(() => {
    if (popupShown || showModal) {
      return
    }
    if (shouldShowBAPPopup(onlyAnonWallet, balance.total)) {
      setPopupShown(true)
      saveBAPPopupShown()
      const popupURL = 'brave_rewards_panel.html#bap-deprecation'
      chrome.braveRewards.openBrowserActionUI(popupURL)
    }
  }, [popupShown, showModal, onlyAnonWallet, balance])

  React.useEffect(() => {
    if (modalRequestInHash()) {
      window.location.hash = ''
    }

    // Attach a hashchange listener while this component is rendered. If the
    // BAP deprecation popup has been opened in the brave rewards extension
    // panel and the user clicks "Learn more", it will update the hash on
    // this page.
    const onHashChange = () => {
      if (modalRequestInHash()) {
        window.location.hash = ''
        setShowModal(true)
      }
    }

    window.addEventListener('hashchange', onHashChange)
    return () => { window.removeEventListener('hashchange', onHashChange) }
  }, [])

  if (!showModal) {
    return null
  }

  const onClose = () => {
    setShowModal(false)
    if (!popupShown) {
      saveBAPAlertDismissed()
    }
  }

  return (
    <WithThemeVariables>
      <LocaleContext.Provider value={locale}>
        <BAPDeprecationAlert onClose={onClose} />
      </LocaleContext.Provider>
    </WithThemeVariables>
  )
}
