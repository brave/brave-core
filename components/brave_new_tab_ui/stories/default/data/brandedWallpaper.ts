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
