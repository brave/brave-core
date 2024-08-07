// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '$web-common/locale'
import * as S from './style'
import { useSelector } from '../../state/hooks'
import getPanelBrowserAPI from '../../api/panel_browser_api'

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
    getPanelBrowserAPI().panelHandler.openVpnUI(intent)
  }

  return (
    <S.Box>
      <S.PanelContent>
        <S.MainLogo name='product-vpn' />
        <S.PanelHeader role='banner'>
          <S.ProductTitle>{getLocale('braveVpn')}</S.ProductTitle>
          <S.PoweredBy>
            <span>{getLocale('braveVpnPoweredBy')}</span>
            <S.GuardianLogo />
          </S.PoweredBy>
        </S.PanelHeader>
        <S.SellingPoints>
          {featureList.map((entry, i) => (
            <S.SellingPoint key={i}>
              <S.SellingPointIcon name='shield-done' />
              <S.SellingPointLabel>
                {entry}
              </S.SellingPointLabel>
            </S.SellingPoint>
          ))}
        </S.SellingPoints>
        <S.ActionArea>
          <S.ActionButton
            slot='actions'
            kind='filled'
            size='large'
            onClick={() => handleClick('checkout')}
          >
            {getLocale('braveVpnBuy')}
          </S.ActionButton>
          <S.ActionLink href="#" onClick={() => handleClick('recover')}>
            {getLocale('braveVpnPurchased')}
          </S.ActionLink>
        </S.ActionArea>
      </S.PanelContent>
      <S.SellGraphic />
    </S.Box>
  )
}

export default SellPanel
