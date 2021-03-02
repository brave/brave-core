// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

export default class AddSiteTileIcon extends React.PureComponent {
  render () {
    return (
      <svg width={'20'} height={'20'} xmlns={'https://www.w3.org/2000/svg'}>
        <path d={'M10 0.244141C9.58579 0.244141 9.25 0.579043 9.25 0.992167V9.22046H1C0.585786 9.22046 0.25 9.55536 0.25 9.96848C0.25 10.3816 0.585786 10.7165 1 10.7165H9.25V18.9448C9.25 19.3579 9.58579 19.6928 10 19.6928C10.4142 19.6928 10.75 19.3579 10.75 18.9448V10.7165H19C19.4142 10.7165 19.75 10.3816 19.75 9.96848C19.75 9.55536 19.4142 9.22046 19 9.22046H10.75V0.992167C10.75 0.579043 10.4142 0.244141 10 0.244141Z'} fillRule={'evenodd'} fill={'#fff'} />
      </svg>
    )
  }
}
