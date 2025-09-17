// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '$web-common/locale'
import * as Styles from './style'
import ContactSupport from '../contact-support'

interface Props {
  stateDescription?: string
}

function PurchaseFailedPanel(props: Props) {
  const [isContactSupportVisible, setContactSupportVisible] =
    React.useState(false)
  const closeContactSupport = () => {
    setContactSupportVisible(false)
  }
  const showContactSupport = () => {
    setContactSupportVisible(!isContactSupportVisible)
  }
  if (isContactSupportVisible) {
    return <ContactSupport onCloseContactSupport={closeContactSupport} />
  }

  const title = props.stateDescription
    ? props.stateDescription
    : getLocale(S.BRAVE_VPN_PURCHASE_FAILED)
  return (
    <Styles.Box>
      <Styles.PanelContent>
        <Styles.EmptyPanelHeader />
        <Styles.StyledAlert
          type='error'
          hideIcon
        >
          {title}
        </Styles.StyledAlert>

        {!isContactSupportVisible && (
          <Styles.StyledActionButton
            slot='actions'
            kind='plain'
            onClick={showContactSupport}
          >
            {getLocale(S.BRAVE_VPN_CONTACT_SUPPORT)}
          </Styles.StyledActionButton>
        )}
      </Styles.PanelContent>
    </Styles.Box>
  )
}

export default PurchaseFailedPanel
