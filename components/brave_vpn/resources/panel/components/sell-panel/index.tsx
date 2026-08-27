// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '$web-common/locale'
import * as Styles from './style'
import { useSelector } from '../../state/hooks'
import getPanelBrowserAPI, { ManageURLType } from '../../api/panel_browser_api'

function SellPanel () {
  const productUrls = useSelector(state => state.productUrls)

  const featureList = React.useMemo(() => ([
    getLocale(S.BRAVE_VPN_FEATURE_1),
    getLocale(S.BRAVE_VPN_FEATURE_2),
    getLocale(S.BRAVE_VPN_FEATURE_3),
    getLocale(S.BRAVE_VPN_FEATURE_4),
    getLocale(S.BRAVE_VPN_FEATURE_5),
    getLocale(S.BRAVE_VPN_FEATURE_6),
    getLocale(S.BRAVE_VPN_FEATURE_7)
  ]), [])

  const handleClick = (intent: ManageURLType) => {
    if (!productUrls) return
    getPanelBrowserAPI().panelHandler.openVpnUI(intent)
  }

  return (
    <Styles.Box>
      <Styles.PanelContent>
        <Styles.PanelHeader role='banner'>
          <Styles.MainLogo name='product-vpn' />
          <Styles.ProductTitle>{getLocale(S.BRAVE_VPN)}</Styles.ProductTitle>
        </Styles.PanelHeader>
        <Styles.PoweredBy>
          <span>{getLocale(S.BRAVE_VPN_POWERED_BY)}</span>
          <Styles.GuardianLogo />
        </Styles.PoweredBy>
        <Styles.SellingPoints>
          {featureList.map((entry, i) => (
            <Styles.SellingPoint key={i}>
              <Styles.SellingPointIcon name='check-normal' />
              <Styles.SellingPointLabel>
                {entry}
              </Styles.SellingPointLabel>
            </Styles.SellingPoint>
          ))}
        </Styles.SellingPoints>
        <Styles.ActionArea>
          <Styles.ActionButton
            slot='actions'
            kind='hero'
            size='medium'
            onClick={() => handleClick(ManageURLType.CHECKOUT)}
          >
            {getLocale(S.BRAVE_VPN_BUY)}
          </Styles.ActionButton>
          <Styles.ActionLink
            href="#"
            onClick={() => handleClick(ManageURLType.RECOVER)}
          >
            {getLocale(S.BRAVE_VPN_HAS_PURCHASED)}
          </Styles.ActionLink>
        </Styles.ActionArea>
      </Styles.PanelContent>
    </Styles.Box>
  )
}

export default SellPanel
