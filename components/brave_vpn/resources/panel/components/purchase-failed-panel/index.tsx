// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { getLocale } from '../../../../../common/locale'
import * as S from '../general'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import ContactSupport from '../contact-support'

interface Props {
  stateDescription?: string
}

function PurchaseFailedPanel (props: Props) {
  const [isContactSupportVisible, setContactSupportVisible] = React.useState(false)
  const closeContactSupport = () => {
    setContactSupportVisible(false)
  }
  const showContactSupport = () => {
    setContactSupportVisible(!isContactSupportVisible)
  }
  if (isContactSupportVisible) {
    return <ContactSupport
      onCloseContactSupport={closeContactSupport}
    />
  }

  const title = props.stateDescription ? props.stateDescription
                                       : getLocale('braveVpnPurchaseFailed')
  return (
    <S.Box>
      <S.PanelContent>
        <S.IconBox>
          <AlertCircleIcon color='#84889C' />
        </S.IconBox>
        <S.Title>{title}</S.Title>
        {!isContactSupportVisible && <S.ButtonText onClick={showContactSupport}>
            {getLocale('braveVpnContactSupport')}
        </S.ButtonText>
        }
      </S.PanelContent>
    </S.Box>
  )
}

export default PurchaseFailedPanel
