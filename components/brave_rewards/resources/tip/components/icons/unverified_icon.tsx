/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export function UnverifiedIcon () {
  return (
    <svg className='icon' fill='none' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 33 33'>
      <g filter='url(#filter0_d)'>
        <path fillRule='evenodd' clipRule='evenodd' d='M16.981 26.507c6.627 0 12-5.457 12-12.189 0-6.731-5.373-12.188-12-12.188-6.628 0-12 5.457-12 12.189 0 6.731 5.373 12.188 12 12.188z' fill='#AEB1C2'/>
        <path d='M28.481 14.319c0 6.462-5.156 11.688-11.5 11.688s-11.5-5.226-11.5-11.689c0-6.462 5.156-11.688 11.5-11.688s11.5 5.226 11.5 11.689z' stroke='#fff'/>
      </g>
      <path fillRule='evenodd' clipRule='evenodd' d='M19.258 18.753a1 1 0 101.413-1.416l-2.584-2.58 2.582-2.587a1 1 0 10-1.415-1.413l-2.582 2.586-2.586-2.583a1 1 0 00-1.413 1.415l2.585 2.583-2.58 2.584a1 1 0 001.414 1.413l2.581-2.584 2.585 2.582z' fill='#fff'/>
      <defs>
        <filter id='filter0_d' x='.981' y='.13' width='32' height='32.377' filterUnits='userSpaceOnUse' colorInterpolationFilters='sRGB'>
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
