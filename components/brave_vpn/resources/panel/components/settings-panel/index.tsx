import * as React from 'react'

import { getLocale } from '../../../../../common/locale'
import { useSelector } from '../../state/hooks'
import * as S from './style'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'

function SettingsPanel () {
  const productUrls = useSelector(state => state.productUrls)

  return (
    <S.Box>
      <S.PanelContent>
        <S.PanelHeader>
          <S.BackButton
            type='button'
            aria-description='Go back to main panel'
          >
            <i><CaratStrongLeftIcon /></i>
            <span>{getLocale('braveVpnSettings')}</span>
          </S.BackButton>
        </S.PanelHeader>
        <S.List>
          <S.Card>
            <S.CardRow>
              <dt>{getLocale('braveVpnStatus')}</dt>
              <dd>Yearly subscription</dd>
            </S.CardRow>
            <S.CardRow>
              <dt>{getLocale('braveVpnExpires')}</dt>
              <dd>1/12/22</dd>
            </S.CardRow>
            <S.CardRow>
              <a href={productUrls?.manage} target='_blank'>
                {getLocale('braveVpnManageSubscription')}
              </a>
            </S.CardRow>
          </S.Card>
          <li>
            <a href={productUrls?.feedback} target='_blank'>
              {getLocale('braveVpnContactSupport')}
            </a>
          </li>
          <li>
            <a href={productUrls?.about} target='_blank'>
              {getLocale('braveVpnAbout')}
              {' '}
              {getLocale('braveVpn')}
            </a>
          </li>
        </S.List>
      </S.PanelContent>
    </S.Box>
  )
}

export default SettingsPanel
