import * as React from 'react'

import { getLocale } from '../../../../../common/locale'
import * as S from '../general'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import ContactSupport from '../contact-support'

function PurchaseFailedPanel () {
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

  return (
    <S.Box>
      <S.PanelContent>
        <S.IconBox>
          <AlertCircleIcon color='#84889C' />
        </S.IconBox>
        <S.Title>{getLocale('braveVpnPurchaseFailed')}</S.Title>
        {!isContactSupportVisible && <S.ButtonText onClick={showContactSupport}>
            {getLocale('braveVpnContactSupport')}
        </S.ButtonText>
        }
      </S.PanelContent>
    </S.Box>
  )
}

export default PurchaseFailedPanel
