/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export function VectorImage () {
  return (
    <svg width='120' height='111' xmlns='http://www.w3.org/2000/svg'>
      <defs>
        <linearGradient x1='9.648%' y1='-75.503%' x2='58.161%' y2='69.005%' id='a'>
          <stop stopColor='#CFD3FA' offset='0%'/>
          <stop stopColor='#2638FA' offset='100%'/>
        </linearGradient>
        <linearGradient x1='0%' y1='50.706%' x2='100%' y2='50.706%' id='b'>
          <stop stopColor='#FB542B' offset='0%'/>
          <stop stopColor='#E3380E' offset='100%'/>
        </linearGradient>
        <linearGradient x1='.643%' y1='49.357%' x2='99.357%' y2='49.357%' id='c'>
          <stop stopColor='#392DD1' offset='0%'/>
          <stop stopColor='#A91B78' offset='100%'/>
        </linearGradient>
        <linearGradient x1='35.192%' y1='0%' x2='146.327%' y2='222.965%' id='d'>
          <stop stopColor='#392DD1' offset='0%'/>
          <stop stopColor='#FF4343' offset='100%'/>
        </linearGradient>
      </defs>
      <g fillRule='nonzero' fill='none'>
        <circle fill='url(#a)' cx='68' cy='104' r='7'/>
        <ellipse fill='#FFF' cx='36.171' cy='7.586' rx='6.96' ry='6.958'/>
        <circle fill='url(#b)' cx='108.69' cy='58.104' r='10.403'/>
        <circle fill='url(#b)' cx='11.711' cy='35.128' r='10.5'/>
        <circle fill='url(#c)' cx='7.211' cy='79.628' r='7'/>
        <circle fill='url(#d)' cx='82.711' cy='10.128' r='8.5'/>
        <path d='M28.447 75.944l18.066-10.551 12.455-21.701V22.769c-.329 0-.657.217-.906.65L43.309 49.126 28.557 74.831c-.249.434-.272.827-.11 1.113' fill='#FF4724'/>
        <path d='M58.968 22.769v20.923l12.454 21.7 18.066 10.552c.163-.286.14-.68-.109-1.113L74.627 49.125 59.874 23.42c-.25-.434-.578-.651-.906-.651' fill='#9E1F63'/>
        <path d='M89.488 75.944L71.422 65.393H46.513L28.448 75.944c.162.288.513.466 1.011.466h59.017c.498 0 .849-.178 1.012-.466' fill='#662D91'/>
        <path fill='#FFF' d='M46.513 65.393h24.909L58.968 43.692z'/>
      </g>
    </svg>
  )
}
