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

  const handleClick = (intent: string) => {
    if (!productUrls) return
    const url = new URL(`?intent=${intent}&product=vpn`, productUrls.manage)
    chrome.tabs.create({ url: url.href })
  }

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
          {featureList.map((entry, i) => (
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
            text={getLocale('braveVpnBuy')}
            onClick={handleClick.bind(null, 'checkout')}
          />
          <a href="#" onClick={handleClick.bind(null, 'recover')}>
            {getLocale('braveVpnPurchased')}
          </a>
        </S.ActionArea>
      </S.PanelContent>
      <S.SellGraphic />
    </S.Box>
  )
}

export default SellPanel
