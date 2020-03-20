/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export default class DisconnectIcon extends React.PureComponent {

  render () {
    return (
      <svg width={'16'} height={'13'} xmlns={'http://www.w3.org/2000/svg'}>
        <defs>
          <path d={'M8.57 10.304a.609.609 0 10.86.861l1.828-1.827a.622.622 0 00.18-.432.604.604 0 00-.18-.431L9.431 6.647a.609.609 0 10-.862.862l.788.788h-7.06a.609.609 0 100 1.219h7.06l-.788.788zm7.742-6.273v9.75A1.22 1.22 0 0115.095 15H7.172a1.22 1.22 0 01-1.219-1.219v-2.437a.609.609 0 111.219 0v2.437h7.922v-9.75H7.172V6.47a.609.609 0 11-1.219 0V4.03a1.22 1.22 0 011.219-1.218h7.922a1.22 1.22 0 011.219 1.218z'} id={'a'}/>
        </defs>
        <use fill={'#84889C'} xlinkHref={'#a'} transform={'translate(-1 -2.5)'} fillRule={'evenodd'}/>
      </svg>
    )
  }
}
