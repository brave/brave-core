// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '$web-common/locale'
import * as S from './style'
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
    : getLocale('braveVpnPurchaseFailed')
  return (
    <S.Box>
      <S.PanelContent>
        <S.EmptyPanelHeader />
        <S.StyledAlert
          type='error'
          mode='full'
          hideIcon
        >
          {title}
        </S.StyledAlert>

        {!isContactSupportVisible && (
          <S.StyledActionButton
            slot='actions'
            kind='plain'
            onClick={showContactSupport}
          >
            {getLocale('braveVpnContactSupport')}
          </S.StyledActionButton>
        )}
      </S.PanelContent>
    </S.Box>
  )
}

export default PurchaseFailedPanel
