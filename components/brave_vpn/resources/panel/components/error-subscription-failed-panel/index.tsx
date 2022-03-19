import * as React from 'react'
import Button from '$web-components/button'

import * as S from './style'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../../common/locale'
import { useSelector } from '../../state/hooks'
import ContactSupport from '../contact-support'

function ErrorSubscriptionFailed () {
  const productUrls = useSelector(state => state.productUrls)
  const [isContactSupportVisible, setContactSupportVisible] = React.useState(false)

  const handleEditPayment = () => {
    chrome.tabs.create({ url: productUrls?.manage })
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
          {getLocale('braveVpnPaymentFailureReason')}
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
