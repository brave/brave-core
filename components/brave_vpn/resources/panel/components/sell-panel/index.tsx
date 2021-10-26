import * as React from 'react'

import { getLocale } from '../../../../../common/locale'
import { useSelector } from '../../state/hooks'
import * as S from './style'
import { Button } from 'brave-ui'

function SellPanel () {
  const productUrls = useSelector(state => state.productUrls)

  const featureList = React.useMemo(() => ([
    getLocale('braveVpnFeature1'),
    getLocale('braveVpnFeature2'),
    getLocale('braveVpnFeature3'),
    getLocale('braveVpnFeature4'),
    getLocale('braveVpnFeature5')
  ]), [])

  return (
    <S.Box>
      <S.PanelContent>
        <S.PanelHeader role='banner'>
          <S.MainLogo />
          <S.ProductTitle>{getLocale('braveVpn')}</S.ProductTitle>
          <S.PoweredBy>
            <span>{getLocale('braveVpnPoweredBy')}</span>
            <S.GuardianLogo />
          </S.PoweredBy>
        </S.PanelHeader>
        <S.List>
          {featureList.map((entry,i) => (
            <li key={i}>
              {entry}
            </li>
          ))}
        </S.List>
        <S.ActionArea>
          <Button
            level='primary'
            type='default'
            brand='rewards'
            text={getLocale('braveVpnBuy').replace('$1', '$4.99/mo')}
          />
          <a href={productUrls?.manage} target='_blank'>
            {getLocale('braveVpnPurchased')}
          </a>
        </S.ActionArea>
      </S.PanelContent>
      <S.SellGraphic />
    </S.Box>
  )
}

export default SellPanel
