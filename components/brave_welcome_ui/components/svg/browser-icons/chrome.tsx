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
    <g clipPath="url(#chrome-a)">
      <path
        d="M130 177.978c27.614 0 50-22.386 50-50s-22.386-50-50-50-50 22.386-50 50 22.386 50 50 50Z"
        fill="#fff"
      />
      <path
        d="M130 78h86.588a99.974 99.974 0 0 0-173.183.012L86.7 153l.038-.01A49.94 49.94 0 0 1 80 127.994a49.936 49.936 0 0 1 6.682-25.01 49.935 49.935 0 0 1 18.307-18.305A49.938 49.938 0 0 1 130 78Z"
        fill="url(#chrome-b)"
      />
      <path
        d="M130 167.583c21.861 0 39.583-17.722 39.583-39.583 0-21.861-17.722-39.583-39.583-39.583-21.861 0-39.583 17.722-39.583 39.583 0 21.861 17.722 39.583 39.583 39.583Z"
        fill="#1A73E8"
      />
      <path
        d="M173.297 153.012 130.003 228a99.976 99.976 0 0 0 49.998-13.393 99.96 99.96 0 0 0 36.6-36.601 99.98 99.98 0 0 0 13.391-49.999 99.981 99.981 0 0 0-13.409-49.994h-86.588l-.01.039a49.938 49.938 0 0 1 50.034 49.96 49.941 49.941 0 0 1-6.722 25Z"
        fill="url(#chrome-c)"
      />
      <path
        d="M86.703 153.013 43.407 78.025a99.975 99.975 0 0 0-13.401 49.996 99.98 99.98 0 0 0 13.398 49.998A99.974 99.974 0 0 0 130.01 228l43.295-74.988-.028-.028a49.94 49.94 0 0 1-43.279 25.052 49.94 49.94 0 0 1-43.295-25.023Z"
        fill="url(#chrome-d)"
      />
    </g>
    <defs>
      <linearGradient
        id="chrome-b"
        x1={43.405}
        y1={90.5}
        x2={216.588}
        y2={90.5}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#D93025" />
        <stop offset={1} stopColor="#EA4335" />
      </linearGradient>
      <linearGradient
        id="chrome-c"
        x1={116.341}
        y1={226.663}
        x2={202.933}
        y2={76.682}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#FCC934" />
        <stop offset={1} stopColor="#FBBC04" />
      </linearGradient>
      <linearGradient
        id="chrome-d"
        x1={140.825}
        y1={221.756}
        x2={54.234}
        y2={71.775}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#1E8E3E" />
        <stop offset={1} stopColor="#34A853" />
      </linearGradient>
      <clipPath id="chrome-a">
        <path fill="#fff" d="M30 28h200v200H30z" />
      </clipPath>
    </defs>
  </svg>
)

export default SvgComponent
