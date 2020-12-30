/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export function VerifiedIcon () {
  return (
    <svg className='icon' viewBox='0 0 32 33'>
      <g filter='url(#filter0_d)'>
        <path fillRule='evenodd' clipRule='evenodd' d='M16 26.46c6.627 0 12-5.458 12-12.19 0-6.73-5.373-12.188-12-12.188S4 7.539 4 14.271c0 6.731 5.373 12.188 12 12.188z' fill='#737ADE'/>
      </g>
      <path d='M10.977 15.047l4.284 3.5 6.055-7' stroke='#fff' strokeWidth='2.8' strokeLinecap='round' strokeLinejoin='round' fill='#737ADE'/>
      <defs>
        <filter id='filter0_d' x='0' y='.082' width='32' height='32.377' filterUnits='userSpaceOnUse' colorInterpolationFilters='sRGB'>
          <feFlood floodOpacity='0' result='BackgroundImageFix'/>
          <feColorMatrix in='SourceAlpha' type='matrix' values='0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 127 0'/>
          <feOffset dy='2'/>
          <feGaussianBlur stdDeviation='2'/>
          <feColorMatrix type='matrix' values='0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0.24 0'/>
          <feBlend mode='normal' in2='BackgroundImageFix' result='effect1_dropShadow'/>
          <feBlend mode='normal' in='SourceGraphic' in2='effect1_dropShadow' result='shape'/>
        </filter>
      </defs>
    </svg>
  )
}
