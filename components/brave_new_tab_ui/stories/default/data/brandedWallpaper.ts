// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import wallpaperImageUrl from '../../../../img/newtab/dummy-branded-wallpaper/background-1.jpg'
import brandingImageUrl from '../../../../img/newtab/dummy-branded-wallpaper/logo.png'

const dummyWallpaper: NewTab.BrandedWallpaper = {
  isSponsored: true,
  wallpaperImageUrl,
  creativeInstanceId: '12345abcde',
  wallpaperId: 'abcde12345',
  logo: {
    image: brandingImageUrl,
    companyName: 'Technikke',
    alt: 'Technikke: For music lovers.',
    destinationUrl: 'https://brave.com'
  }
}

export default dummyWallpaper
