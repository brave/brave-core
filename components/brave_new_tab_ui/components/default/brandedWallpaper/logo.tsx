// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import createWidget from '../widget/index'

import * as Styled from './logo-style'
import { OpenNewIcon } from 'brave-ui/components/icons'
import BrandedWallpaperNotification from '../rewards/brandedWallpaperNotification'

interface Props {
  data: NewTab.BrandedWallpaperLogo
  showBrandedWallpaperNotification: boolean
  brandedWallpaperData?: NewTab.BrandedWallpaper
  onClickLogo: () => void
  onDisableBrandedWallpaper: () => void
  onDismissBrandedWallpaperNotification: (isUserAction: boolean) => void
}

class Logo extends React.PureComponent<Props, {}> {

  dismissNotification = () => {
    this.props.onDismissBrandedWallpaperNotification(true)
  }

  renderNotifications = () => {
    const {
      brandedWallpaperData,
      onDisableBrandedWallpaper,
      showBrandedWallpaperNotification
    } = this.props

    if (!showBrandedWallpaperNotification) {
      return null
    }

    return (
      <BrandedWallpaperNotification
        order={0}
        isOrphan={true}
        onStartRewards={undefined}
        brandedWallpaperData={brandedWallpaperData}
        onDismissNotification={this.dismissNotification}
        onHideSponsoredImages={onDisableBrandedWallpaper}
      />
    )
  }

  render () {
    const { data, onClickLogo } = this.props
    return (
      <>
        {this.renderNotifications()}
        <Styled.Image src={data.image} alt={data.alt} />
        <Styled.Anchor href={data.destinationUrl} title={data.alt} onClick={onClickLogo}>
          <Styled.Indicator><OpenNewIcon /></Styled.Indicator>
        </Styled.Anchor>
      </>
    )
  }
}

export default createWidget(Logo)
