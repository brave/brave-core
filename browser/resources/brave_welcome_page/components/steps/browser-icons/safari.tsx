/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

const SvgComponent = (props: React.SVGProps<SVGSVGElement>) => (
  <svg
    viewBox="0 0 256 256"
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
    {...props}
  >
    <g clipPath="url(#safari-a)">
      <g opacity={0.53} filter="url(#safari-b)">
        <path
          d="M222.962 135.305a88.9 88.9 0 0 1-7.261 35.15c-4.794 11.143-11.82 21.269-20.677 29.798s-19.372 15.295-30.945 19.91a98.576 98.576 0 0 1-36.502 6.992 98.577 98.577 0 0 1-36.502-6.992c-11.573-4.615-22.088-11.381-30.945-19.91-8.858-8.529-15.884-18.655-20.677-29.798a88.908 88.908 0 0 1-7.26-35.15c0-24.36 10.049-47.722 27.937-64.948 17.888-17.225 42.149-26.902 67.447-26.902a98.577 98.577 0 0 1 36.502 6.992c11.573 4.616 22.088 11.381 30.945 19.91 8.857 8.53 15.883 18.655 20.677 29.799a88.89 88.89 0 0 1 7.261 35.149Z"
          fill="#000"
        />
      </g>
      <path
        d="M226.588 127.195a99.014 99.014 0 0 1-29.001 70.015 99.02 99.02 0 0 1-32.123 21.464 99.016 99.016 0 0 1-37.892 7.537 99.016 99.016 0 1 1 0-198.032 99.015 99.015 0 0 1 70.015 29 99.007 99.007 0 0 1 21.464 32.124 99.017 99.017 0 0 1 7.537 37.892v0Z"
        fill="url(#safari-c)"
        stroke="#CDCDCD"
        strokeWidth={0.351}
        strokeLinecap="round"
        strokeLinejoin="round"
      />
      <path
        d="M218.83 127.195a91.256 91.256 0 0 1-91.258 91.257 91.255 91.255 0 1 1 0-182.514 91.258 91.258 0 0 1 91.258 91.257Z"
        fill="url(#safari-d)"
      />
      <path
        d="M127.572 40.696c-.737 0-1.33.594-1.33 1.33v15.348c0 .737.593 1.33 1.33 1.33a1.328 1.328 0 0 0 1.331-1.33V42.027a1.328 1.328 0 0 0-1.331-1.33Z"
        fill="#F4F2F3"
      />
      <g opacity={0.409} filter="url(#safari-e)">
        <path
          d="m189.93 73.975-72.794 42.293-46.025 72.457 67.331-49.871 51.488-64.879Z"
          fill="#000"
        />
      </g>
      <path
        d="m138.005 138.12-20.865-21.85 74.016-49.791-53.151 71.641Z"
        fill="#FF5150"
      />
      <path
        d="m138.005 138.12-20.865-21.85-53.151 71.641 74.016-49.791Z"
        fill="#F1F1F1"
      />
      <path
        opacity={0.243}
        d="m63.989 187.911 74.016-49.791 53.151-71.641L63.989 187.91Z"
        fill="#000"
      />
    </g>
    <defs>
      <filter
        id="safari-b"
        x={23.239}
        y={34.502}
        width={208.677}
        height={201.607}
        filterUnits="userSpaceOnUse"
        colorInterpolationFilters="sRGB"
      >
        <feFlood floodOpacity={0} result="BackgroundImageFix" />
        <feBlend in="SourceGraphic" in2="BackgroundImageFix" result="shape" />
        <feGaussianBlur
          stdDeviation={4.477}
          result="effect1_foregroundBlur_580_10783"
        />
      </filter>
      <filter
        id="safari-e"
        x={68.566}
        y={71.43}
        width={123.908}
        height={119.839}
        filterUnits="userSpaceOnUse"
        colorInterpolationFilters="sRGB"
      >
        <feFlood floodOpacity={0} result="BackgroundImageFix" />
        <feBlend in="SourceGraphic" in2="BackgroundImageFix" result="shape" />
        <feGaussianBlur
          stdDeviation={1.272}
          result="effect1_foregroundBlur_580_10783"
        />
      </filter>
      <radialGradient
        id="safari-d"
        cx={0}
        cy={0}
        r={1}
        gradientUnits="userSpaceOnUse"
        gradientTransform="translate(127.984 113.888) scale(99.0158)"
      >
        <stop stopColor="#06C2E7" />
        <stop offset={0.25} stopColor="#0DB8EC" />
        <stop offset={0.5} stopColor="#12AEF1" />
        <stop offset={0.75} stopColor="#1F86F9" />
        <stop offset={1} stopColor="#107DDD" />
      </radialGradient>
      <linearGradient
        id="safari-c"
        x1={127.569}
        y1={226.209}
        x2={127.569}
        y2={28.178}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#BDBDBD" />
        <stop offset={1} stopColor="#fff" />
      </linearGradient>
      <clipPath id="safari-a">
        <path fill="#fff" d="M23 28h209.144v208H23z" />
      </clipPath>
    </defs>
  </svg>
)

export default SvgComponent

