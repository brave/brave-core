// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Button from '$web-components/button'

import * as S from './style'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../../common/locale'
import ContactSupport from '../contact-support'
import getPanelBrowserAPI from '../../api/panel_browser_api'

function ErrorSubscriptionFailed () {
  const [isContactSupportVisible, setContactSupportVisible] = React.useState(false)

  const handleEditPayment = () => {
    getPanelBrowserAPI().panelHandler.openVpnUI('manage')
  }

  const handleContactSupport = () => setContactSupportVisible(true)
  const closeContactSupport = () => setContactSupportVisible(false)

  if (isContactSupportVisible) {
    return (<ContactSupport
      onCloseContactSupport={closeContactSupport}
    />)
  }

  return (
    <S.Box>
      <S.PanelContent>
        <S.IconBox>
          <AlertCircleIcon color='#84889C' />
        </S.IconBox>
        <S.ReasonTitle>
          {getLocale('braveVpnPaymentFailure')}
        </S.ReasonTitle>
        <S.ReasonDesc>
          {getLocale('braveVpnPaymentFailureReason').replace('$1', getLocale('braveVpn'))}
        </S.ReasonDesc>
        <S.ActionArea>
          <Button
            type={'submit'}
            isPrimary
            isCallToAction
            onClick={handleEditPayment}
          >
            {getLocale('braveVpnEditPaymentMethod')}
          </Button>
          <S.ButtonText onClick={handleContactSupport}>
            {getLocale('braveVpnContactSupport')}
          </S.ButtonText>
        </S.ActionArea>
      </S.PanelContent>
    </S.Box>
  )
}

export default ErrorSubscriptionFailed
