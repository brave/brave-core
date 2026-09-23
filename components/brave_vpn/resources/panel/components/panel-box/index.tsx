// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import * as Styles from './style'

interface Props {
  children: React.ReactNode
}

function PanelBox (props: Props) {
  return (
    <Styles.Box>
      {props.children}
      <Styles.WavesContainer>
        <Styles.Waves />
      </Styles.WavesContainer>
    </Styles.Box>
  )
}

export default PanelBox
