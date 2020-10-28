/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'

const styles = {
  root: styled.div`
    position: fixed;
    top: 0;
    left: 0;
    bottom: 0;
    right: 0;
    background: rgba(0, 0, 0, 0.33);
    z-index: 9999;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 0 20px;
  `,

  topSpacer: styled.div`
    flex: 45 0 auto;
  `,

  content: styled.div`
    flex: 0 0 auto;
  `,

  bottomSpacer: styled.div`
    flex: 55 0 auto;
  `
}

interface Props {
  children: React.ReactNode
}

export function Modal (props: Props) {
  return (
    <styles.root>
      <styles.topSpacer />
      <styles.content>
        {props.children}
      </styles.content>
      <styles.bottomSpacer />
    </styles.root>
  )
}
