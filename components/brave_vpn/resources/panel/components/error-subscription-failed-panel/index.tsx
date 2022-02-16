import * as React from 'react'
import { Button } from 'brave-ui'

import * as S from './style'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../../common/locale'
import { useSelector } from '../../state/hooks'

function ErrorSubscriptionFailed () {
  const productUrls = useSelector(state => state.productUrls)

  const handleEditPayment = () => {
    chrome.tabs.create({ url: productUrls?.manage })
  }

  const handleContactSupport = () => {
    // TODO (nullhook): Add another ViewType state
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
              level='primary'
              type='accent'
              brand='rewards'
              text={getLocale('braveVpnEditPaymentMethod')}
              onClick={handleEditPayment}
            />
            <Button
              level='tertiary'
              type='accent'
              brand='rewards'
              text={getLocale('braveVpnContactSupport')}
              onClick={handleContactSupport}
            />
        </S.ActionArea>
      </S.PanelContent>
    </S.Box>
  )
}

export default ErrorSubscriptionFailed
