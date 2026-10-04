// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

const SvgComponent = (props: any) => (
  <svg
    viewBox="0 0 256 256"
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
    {...props}
  >
    <g clipPath="url(#chromium-a)">
      <path
        d="M128 128.001 171.303 153 128 228c55.229 0 100-44.771 100-99.999 0-18.22-4.897-35.287-13.411-50h-86.59l.001 50Z"
        fill="url(#chromium-b)"
      />
      <path
        d="M127.999 28c-37.013 0-69.304 20.123-86.595 50.012L84.697 153l43.302-24.999v-50h86.59C197.295 48.118 165.008 28 127.999 28Z"
        fill="url(#chromium-c)"
      />
      <path
        d="M28 128.001C28 183.229 72.77 228 128 228l43.303-75L128 128.001 84.698 153 41.405 78.012C32.895 92.722 28 109.782 28 128"
        fill="url(#chromium-d)"
      />
      <path
        d="M178.001 128.001c0 27.614-22.386 50-50.001 50-27.614 0-50-22.386-50-50s22.386-50 50-50c27.615 0 50.001 22.386 50.001 50"
        fill="#fff"
      />
      <path
        d="M168.626 128.001c0 22.436-18.189 40.625-40.626 40.625-22.436 0-40.625-18.189-40.625-40.625 0-22.437 18.189-40.626 40.625-40.626 22.437 0 40.626 18.189 40.626 40.626"
        fill="url(#chromium-e)"
      />
    </g>
    <defs>
      <linearGradient
        id="chromium-b"
        x1={178.213}
        y1={77.794}
        x2={177.999}
        y2={228}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#AFCCFB" />
        <stop offset={1} stopColor="#8BB5F8" />
      </linearGradient>
      <linearGradient
        id="chromium-c"
        x1={113.496}
        y1={29.524}
        x2={121.766}
        y2={163.907}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#1972E7" />
        <stop offset={1} stopColor="#1969D5" />
      </linearGradient>
      <linearGradient
        id="chromium-d"
        x1={99.651}
        y1={78.012}
        x2={99.655}
        y2={228.002}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#659CF6" />
        <stop offset={1} stopColor="#4285F4" />
      </linearGradient>
      <linearGradient
        id="chromium-e"
        x1={127.586}
        y1={87.53}
        x2={128}
        y2={168.626}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#3680F0" />
        <stop offset={1} stopColor="#2678EC" />
      </linearGradient>
      <clipPath id="chromium-a">
        <path fill="#fff" d="M28 28h200v200H28z" />
      </clipPath>
    </defs>
  </svg>
)

export default SvgComponent
