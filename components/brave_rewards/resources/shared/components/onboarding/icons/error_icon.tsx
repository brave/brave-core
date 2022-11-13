/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export function ErrorIcon () {
  return (
    <svg className='icon' fill='none' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>
      <path fillRule='evenodd' clipRule='evenodd' d='M12 24C5.383 24 0 18.617 0 12S5.383 0 12 0s12 5.383 12 12-5.383 12-12 12Zm0-5c-.26 0-.52-.11-.71-.29-.181-.19-.29-.45-.29-.71 0-.27.109-.52.29-.71.37-.37 1.04-.37 1.42 0 .18.19.29.44.29.71 0 .26-.11.52-.29.71-.19.18-.45.29-.71.29Zm0-4c-.553 0-1-.448-1-1V6c0-.552.447-1 1-1 .553 0 1 .448 1 1v8c0 .552-.447 1-1 1Z' fill='#E32444'/>
    </svg>
  )
}
