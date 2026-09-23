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
    <g clipPath="url(#opera-a)">
      <path
        d="M97.055 179.978c-10.396-12.276-17.127-30.399-17.578-50.76v-4.436c.451-20.361 7.182-38.484 17.578-50.76 13.499-17.503 33.539-28.614 55.911-28.614 13.762 0 26.659 4.211 37.676 11.524C174.098 42.118 152.29 33.094 128.357 33H128c-51.907 0-94 42.093-94 94 0 50.422 39.687 91.556 89.526 93.887 1.485.075 2.97.113 4.474.113 24.064 0 46.022-9.043 62.642-23.914-11.017 7.295-23.895 11.506-37.676 11.506-22.372 0-42.412-11.111-55.91-28.614Z"
        fill="url(#opera-b)"
      />
      <path
        d="M97.055 74.022c8.629-10.19 19.759-16.319 31.941-16.319 27.373 0 49.538 31.02 49.538 69.297s-22.184 69.297-49.538 69.297c-12.163 0-23.312-6.148-31.94-16.319 13.498 17.503 33.538 28.614 55.91 28.614 13.762 0 26.659-4.211 37.676-11.506C209.893 179.866 222 154.843 222 127c0-27.843-12.107-52.866-31.358-70.068-11.017-7.313-23.895-11.524-37.676-11.524-22.372 0-42.412 11.11-55.91 28.614Z"
        fill="url(#opera-c)"
      />
    </g>
    <defs>
      <linearGradient
        id="opera-b"
        x1={112.325}
        y1={36.065}
        x2={112.325}
        y2={218.264}
        gradientUnits="userSpaceOnUse"
      >
        <stop offset={0.3} stopColor="#FF1B2D" />
        <stop offset={0.438} stopColor="#FA1A2C" />
        <stop offset={0.594} stopColor="#ED1528" />
        <stop offset={0.758} stopColor="#D60E21" />
        <stop offset={0.927} stopColor="#B70519" />
        <stop offset={1} stopColor="#A70014" />
      </linearGradient>
      <linearGradient
        id="opera-c"
        x1={159.529}
        y1={46.804}
        x2={159.529}
        y2={207.95}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#9C0000" />
        <stop offset={0.7} stopColor="#FF4B4B" />
      </linearGradient>
      <clipPath id="opera-a">
        <path fill="#fff" d="M34 33h188v188H34z" />
      </clipPath>
    </defs>
  </svg>
)

export default SvgComponent
