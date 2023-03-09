/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export function YoutubeIcon () {
  return (
    <svg className='icon' fill='none' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 18 18'>
      <path fill='none' d='M1.5 5.25h15v7.5h-15z'/>
      <path fillRule='evenodd' clipRule='evenodd' d='M16.0128 3.0103c.7741.21017 1.3864.82899 1.5944 1.61127C17.9884 6.04602 18 9 18 9s0 2.9657-.3813 4.3784c-.2079.7823-.8202 1.4011-1.5943 1.6113C14.6264 15.375 9 15.375 9 15.375s-5.62644 0-7.02439-.3853c-.77407-.2102-1.386393-.829-1.594352-1.6113C0 11.954 0 9 0 9s0-2.95398.369705-4.36676c.207959-.78228.820285-1.4011 1.594355-1.61126C3.362 2.63668 8.98845 2.625 8.98845 2.625s5.62645 0 7.02435.3853Zm-3.9867 5.99881L7.49998 11.6433V6.37496l4.52612 2.63415Z' fill='currentColor'/>
    </svg>
  )
}

export function TwitterIcon () {
  return (
    <svg className='icon' fill='none' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 18 18'>
      <path d='M18 3.23844c-.6279.31896-1.3605.5316-2.093.63791.7849-.47843 1.3604-1.22265 1.6744-2.12636-.7326.42527-1.5174.74423-2.3547.90371-1.1696-1.21703-2.9895-1.50386-4.4671-.70407-1.47762.7998-2.25842 2.49431-1.91658 4.1594-2.93023-.15948-5.75581-1.54161-7.58721-3.93377-.503321.81672-.664847 1.80379-.448704 2.74202.216144.93824.792064 1.74996 1.599874 2.25493-.57558 0-1.15117-.10632-1.674422-.37211.052326 1.75425 1.255812 3.2959 2.930232 3.668-.52326.1594-1.09884.1594-1.67442.0531.52326 1.5416 1.93605 2.6048 3.50582 2.658C3.97674 14.455 1.93605 15.0397 0 14.7208c3.3005 2.2375 7.56581 2.3748 10.9992.3542 3.4333-2.0206 5.438-5.84787 5.1694-9.86967.7326-.53159 1.3605-1.1695 1.8314-1.96689Z' fill='currentColor'/>
    </svg>
  )
}

export function TwitchIcon () {
  return (
    <svg className='icon' fill='none' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 18 18'>
      <path d='M4.5.75h12l-.375 7.875L10.5 14.25h-6V.75Z' fill='none'/>
      <path fillRule='evenodd' clipRule='evenodd' d='M1.28613 3.21429 4.50042 0H16.7147v9l-5.7857 5.7857H8.35756L5.14328 18v-3.2143H1.28613V3.21429ZM12.8576 10.9286l2.5714-2.57146V1.28571H5.14328v9.64289h2.89285v2.25l2.24997-2.25h2.5715Zm.6429-7.39291h-1.2857v3.85715h1.2857V3.53569Zm-3.53573.00004H8.67905v3.85714h1.28572V3.53573Z' fill='currentColor'/>
    </svg>
  )
}

export function getSocialIcon (platform: string) {
  switch (platform) {
    case 'youtube': return YoutubeIcon
    case 'twitter': return TwitterIcon
    case 'twitch': return TwitchIcon
  }
  return null
}
