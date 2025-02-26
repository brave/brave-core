// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color, spacing } from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'

export const BraveIconCircle = styled.div`
  align-items: center;
  background-position: center;
  background-repeat: no-repeat;
  background-size: 2.5em;
  border-radius: 50%;
  border: #E3E3E8 1px solid;
  display: flex;
  min-height: 4.5em;
  justify-content: center;
  margin-inline-end: 1.5em;
  min-width: 4.5em;
  flex-grow: 0;
`

export const BraveIconWrapper = styled.div`
  transform scale(2);
  display: inline-block;
`

export const Card = styled.div`
  background-color: ${color.container.background};
  border: none;
  overflow: hidden;
  padding: ${spacing.l} ;
`

export const Row = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
`

export const Col = styled.div`
  display: flex;
  flex-direction: column;
`

export const AccountRow = styled(Row)`
  justify-content: space-between;
  align-items: center;
  padding: 20px 0px 20px;
`

export const SignupRow = styled(Row)`
  justify-content: space-between;
  align-items: start;
`
export const MainEmailTextContainer = styled(Col)`
  justify-content: center;
  cursor: default;
  user-select: none;
`

export const MainEmail = styled.div`
  font-size: 130%;
  font-weight: 600;
  padding-bottom: 6px;
`

export const MainEmailDescription = styled.div`
  font-size: 115%;
`

export const ManageAccountLink = styled.a`
  display: flex;
  flex-direction: row;
  align-items: center;
  font-size: 120%;
  align-items: center;
  color: ${color.text.secondary};
  text-decoration: none;
`

export const MenuButton = styled(Button)`
  --leo-button-padding: 0;
  flex-grow: 0;
`

export const AliasItemRow = styled(Row)`
  margin: 0px;
  font-size: 125%;
  padding: 18px 0px 18px 25px;
  border-top: ${color.legacy.divider1} 1px solid;
  justify-content: space-between;
`

export const AliasAnnotation = styled.div`
  font-size: 80%;
  font-weight: 400;
  padding-top: 0.25em;
  color: rgb(80, 80, 80);
`

export const AliasControls = styled(Row)`
  height: 1.5em;
  user-select: none;
  align-items: top;
`

export const AliasListIntro = styled(Row)`
  margin-bottom: 20px;
  justify-content: space-between;
`


export const Modal = styled(Col)`
  border-radius: var(--cr-card-border-radius);
  background-color: white;
  z-index: 2;
  border: none;
  opacity: 100%;
  position: fixed;
  top: 50%;
  left: 50%;
  transform: translate(-50%,-50%);
  justify-content: flex-start;
`

export const InnerModal = styled.div`
  width: 42em;
  margin: 1em 2em;
`

export const CloseButton = styled.span`
  cursor: pointer;
  position: absolute;
  top: 1.75em;
  right: 1.75em;
`

export const EmailContainer = styled.div`
  cursor: pointer;
`

export const GeneratedEmailContainer = styled(Row)`
  font-size: 135%;
  background-color: #f4f4f4;
  border-radius: 0.5em;
  padding: 0em 0em 0em 0.75em;
  margin: 0.25em 0em;
  justify-content: space-between;
  height: 2.6em;
`

export const ButtonRow = styled(Row)`
  justify-content: space-between;
  margin: 1em 0em;
`

export const ModalSectionCol = styled(Col)`
  justify-content: flex-start;
  margin: 1em 0em;
`

export const GrayOverlay = styled.div`
  background-color: rgb(50,50,50,0.5);
  position: fixed;
  z-index: 1;
  left: 0em;
  right: 0em;
  top: 0em;
  bottom: 0em;
`

export const CopyButtonWrapper = styled.div`
  cursor: pointer;
  color: #666;
  &:hover {
    background-color: #EEE;
  }
  padding: 0.25em;
  border-radius: 0.5em;
`
