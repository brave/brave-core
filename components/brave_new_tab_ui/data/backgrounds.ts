/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const images: NewTab.BraveBackground[] = [{
  'type': 'brave',
  'wallpaperImageUrl': 'dylan-malval_sea-min.webp',
  'author': 'Dylan Malval',
  'link': 'https://www.instagram.com/vass_captures/',
  'originalUrl': 'Contributor sent the hi-res version through email',
  'license': 'used with permission'
}]
// If you change the size of this array (e.g. adding a new background, adding a new property),
// then you must also update `script/generate_licenses.py`

export const updateImages = (newImages: NewTab.BraveBackground[]) => {
  if (!newImages.length) {
    // This can happen when the component for NTP is not downloaded yet on
    // a fresh profile.
    return
  }

  images.splice(0, images.length, ...newImages)
}

export const defaultSolidBackgroundColor = '#151E9A'

export const solidColorsForBackground: NewTab.ColorBackground[] = [
  '#5B5C63', '#000000', '#151E9A', '#2197F9', '#1FC3DC', '#086582', '#67D4B4', '#077D5A',
  '#3C790B', '#AFCE57', '#F0CB44', '#F28A29', '#FC798F', '#C1226E', '#FAB5EE', '#C0C4FF',
  '#9677EE', '#5433B0', '#4A000C'
].map((color): NewTab.ColorBackground => ({ 'type': 'color', 'wallpaperColor': color }))

export const defaultGradientColor = 'linear-gradient(125.83deg, #392DD1 0%, #A91B78 99.09%)'

export const gradientColorsForBackground: NewTab.ColorBackground[] = [
  'linear-gradient(125.83deg, #392DD1 0%, #A91B78 99.09%)',
  'linear-gradient(125.83deg, #392DD1 0%, #22B8CF 99.09%)',
  'linear-gradient(90deg, #4F30AB 0.64%, #845EF7 99.36%)',
  'linear-gradient(126.47deg, #A43CE4 16.99%, #A72B6D 86.15%)',
  'radial-gradient(69.45% 69.45% at 89.46% 81.73%, #641E0C 0%, #500F39 43.54%, #060141 100%)',
  'radial-gradient(80% 80% at 101.61% 76.99%, #2D0264 0%, #030023 100%)',
  'linear-gradient(128.12deg, #43D4D4 6.66%, #1596A9 83.35%)',
  'linear-gradient(323.02deg, #DD7131 18.65%, #FBD460 82.73%)',
  'linear-gradient(128.12deg, #4F86E2 6.66%, #694CD9 83.35%)',
  'linear-gradient(127.39deg, #851B6A 6.04%, #C83553 86.97%)',
  'linear-gradient(130.39deg, #FE6F4C 9.83%, #C53646 85.25%)'
].map((color): NewTab.ColorBackground => ({ 'type': 'color', 'wallpaperColor': color }))
