/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { CloseStrokeIcon } from 'brave-ui/components/icons'

import { LocaleContext } from '../localeContext'
import { DialogCloseReason } from '../../../../checkout/lib/interfaces'

import {
  MainPanel,
  Content,
  TopBar,
  TitleContainer,
  DialogTitleIcon,
  CloseButton,
  BatText
} from './style'

export interface DialogFrameProps {
  onClose: (reason: DialogCloseReason) => void
  children: React.ReactNode
  showTitle?: boolean
  showBackground?: boolean
  reason: DialogCloseReason
}

export function DialogFrame (props: DialogFrameProps) {
  const locale = React.useContext(LocaleContext)

  const onClose = () => {
    props.onClose(props.reason)
  }

  return (
    <MainPanel showBackground={props.showBackground}>
      <TopBar>
        <TitleContainer>
        {
          !props.showTitle ? null : <>
            <DialogTitleIcon />
            <BatText>{locale.get('bat')}</BatText> {locale.get('checkout')}
          </>
        }
        </TitleContainer>
        <CloseButton onClick={onClose}>
          <CloseStrokeIcon />
        </CloseButton>
      </TopBar>
      <Content>
        {props.children}
      </Content>
    </MainPanel>
  )
}
