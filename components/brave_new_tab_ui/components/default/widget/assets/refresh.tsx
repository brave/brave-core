/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export default class RefreshIcon extends React.PureComponent {
  render () {
    return (
      <svg width={'15'} height={'18'} xmlns={'http://www.w3.org/2000/svg'}>
        <path d={'M15 9c0 3.436-2.218 6.507-5.393 7.468a.75.75 0 11-.434-1.436C11.72 14.262 13.5 11.78 13.5 9c0-2.607-1.523-4.726-3.829-5.584l.75 1.499a.75.75 0 11-1.342.67l-1.5-3a.75.75 0 01.336-1.005l3-1.5a.749.749 0 11.67 1.34l-1.256.629C13.122 3.162 15 5.838 15 9zm-7.915 7.421l-3 1.5a.749.749 0 01-1.006-.336.75.75 0 01.336-1.006l1.255-.627C1.878 14.84 0 12.164 0 9c0-3.435 2.218-6.505 5.393-7.468a.75.75 0 01.435 1.436C3.28 3.74 1.5 6.221 1.5 9c0 2.61 1.523 4.729 3.829 5.586l-.75-1.5a.75.75 0 011.341-.671l1.5 3a.75.75 0 01-.335 1.006z'} fillRule={'evenodd'}/>
      </svg>
    )
  }
}
