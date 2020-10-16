/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export function VerifiedIcon () {
  return (
    <svg className='icon' fill='none' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 33 34'>
      <g filter='url(#filter0_d)'>
        <path fillRule='evenodd' clipRule='evenodd' d='M16.661 27.177c6.627 0 12-5.457 12-12.188 0-6.732-5.373-12.189-12-12.189s-12 5.457-12 12.189c0 6.731 5.373 12.188 12 12.188z' fill='#4C54D2'/>
        <path d='M28.161 14.989c0 6.463-5.156 11.688-11.5 11.688s-11.5-5.225-11.5-11.688c0-6.463 5.156-11.689 11.5-11.689s11.5 5.226 11.5 11.689z' stroke='#fff'/>
      </g>
      <path d='M12.639 14.989l3.333 2.723 4.712-5.447' stroke='#fff' strokeWidth='1.813' strokeLinecap='round' strokeLinejoin='round'/>
      <defs>
        <filter id='filter0_d' x='.661' y='.8' width='32' height='32.377' filterUnits='userSpaceOnUse' colorInterpolationFilters='sRGB'>
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
