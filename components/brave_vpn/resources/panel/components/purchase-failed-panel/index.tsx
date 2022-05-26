import * as React from 'react'

import { getLocale } from '../../../../../common/locale'
import * as S from '../styles/style'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import ContactSupport from '../contact-support'

function PurchaseFailedPanel () {
  const [isContactSupportVisible, setContactSupportVisible] = React.useState(false)
  const closeContactSupport = () => {
    console.log('isContactSupportVisible', isContactSupportVisible)
    setContactSupportVisible(false)
  }
  const showContactSupport = () => {
    setContactSupportVisible(!isContactSupportVisible)
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
        {isContactSupportVisible &&
          <ContactSupport
            onCloseContactSupport={closeContactSupport}
          />
        }
      </S.PanelContent>
    </S.Box>
  )
}

export default PurchaseFailedPanel
