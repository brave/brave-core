// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  CloseButton,
  StyledSearchPromotionWrapper,
  StyledSearchPromotion,
  StyledSearchPromotionPopup,
  StyledSearchPromotionPopupTitle,
  StyledSearchPromotionPopupTitleWrapper,
  StyledSearchPromotionPopupDesc,
  StyledSearchPromotionPopupBottomWrapper,
  StyledSearchPromotionPopupBottom
} from './style'
import SearchBox from './searchBox'
import { getLocale } from '../../../../common/locale'

function ColorBar () {
  return (
    <svg width="640" height="8" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M138.661 -2H-2.19978L-7.38965 12.8108H126.35L138.661 -2Z" fill="#9F42E6"/>
      <path d="M184.494 -1.89185H121.05L118.402 12.8109H184.494V-1.89185Z" fill="#444DD0"/>
      <path d="M316.718 -2.24341H181.245L175.59 12.7127H316.718V-2.24341Z" fill="#4468EA"/>
      <path d="M338.051 -2.24341H262.564L267.119 12.7127H338.051V-2.24341Z" fill="#6AB6F4"/>
      <path d="M528.41 -2.24341H305.231L316.152 12.7127H506.092L528.41 -2.24341Z" fill="#6AB6F4"/>
      <path d="M607.18 -2.24341H371.846L362.667 12.7127H607.18V-2.24341Z" fill="#FF4724"/>
      <path d="M643.282 -2.24341H554.667L564.179 12.7127H643.282V-2.24341Z" fill="#9E1F63"/>
    </svg>
  )
}

function CloseIcon () {
  return (
    <svg width="14" height="14" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M2.17957 0.779147C1.80963 0.406951 1.20984 0.406951 0.8399 0.779147C0.469961 1.15134 0.469962 1.75479 0.839901 2.12699L5.62449 6.94077L0.722157 11.873C0.352218 12.2452 0.352217 12.8487 0.722156 13.2209C1.09209 13.593 1.69188 13.593 2.06182 13.2209L6.96415 8.28861L11.8664 13.2208C12.2363 13.593 12.8361 13.593 13.2061 13.2208C13.576 12.8486 13.576 12.2451 13.2061 11.8729L8.30382 6.94077L13.0883 2.12709C13.4582 1.75489 13.4582 1.15144 13.0883 0.779245C12.7184 0.407049 12.1186 0.407049 11.7486 0.779245L6.96415 5.59293L2.17957 0.779147Z" fill="#8C90A1"/>
    </svg>
  )
}

function Arrow () {
  return (
    <svg width="19" height="24" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path fillRule="evenodd" clipRule="evenodd" d="M8.5 20.7587L8.5 1C8.5 0.447715 8.94771 0 9.5 0C10.0523 0 10.5 0.447715 10.5 1L10.5 20.7587L16.7652 13.9712C17.1398 13.5654 17.7724 13.5401 18.1783 13.9147C18.5841 14.2893 18.6094 14.922 18.2348 15.3278L10.7248 23.4638C10.0648 24.1787 8.93516 24.1787 8.27519 23.4638L0.765186 15.3278C0.390587 14.922 0.415899 14.2893 0.821724 13.9147C1.22755 13.5401 1.86021 13.5654 2.23481 13.9712L8.5 20.7587Z" fill="#CD9CF2"/>
    </svg>
  )
}

interface Props {
  textDirection: string
  onClose: () => void
  onDismiss: () => void
  onTryBraveSearch: (input: string, openNewTab: boolean) => void
}

export default class SearchPromotion extends React.PureComponent<Props, {}> {
  searchPromotionRef: React.RefObject<any>
  constructor (props: Props) {
    super(props)
    this.searchPromotionRef = React.createRef()
  }

  componentDidMount () {
    document.addEventListener('mousedown', this.handleClickOutside)
    document.addEventListener('keydown', this.onKeyPressSettings)
  }

  componentWillUnmount () {
    document.removeEventListener('mousedown', this.handleClickOutside)
    document.removeEventListener('keydown', this.onKeyPressSettings)
  }

  onKeyPressSettings = (event: KeyboardEvent) => {
    if (event.key === 'Escape') {
      this.props.onClose()
    }
  }

  handleClickOutside = (event: Event) => {
    // Close promotion when user clicks outside of promotion UI.
    if (
      this.searchPromotionRef &&
      this.searchPromotionRef.current &&
      !this.searchPromotionRef.current.contains(event.target)
    ) {
      this.props.onClose()
    }
  }

  render () {
    return (
      <StyledSearchPromotionWrapper>
        <StyledSearchPromotion
          ref={this.searchPromotionRef}
        >
          <StyledSearchPromotionPopup>
            <CloseButton onClick={this.props.onDismiss}>
              <CloseIcon/>
            </CloseButton>
            <ColorBar />
            <StyledSearchPromotionPopupTitleWrapper>
              <StyledSearchPromotionPopupTitle>
                {getLocale('searchPromotionNTPPopupTitle1')}
              </StyledSearchPromotionPopupTitle>
              <StyledSearchPromotionPopupTitle>
                {getLocale('searchPromotionNTPPopupTitle2')}
              </StyledSearchPromotionPopupTitle>
            </StyledSearchPromotionPopupTitleWrapper>
            <StyledSearchPromotionPopupDesc>
              {getLocale('searchPromotionNTPPopupDesc')}
            </StyledSearchPromotionPopupDesc>
            <StyledSearchPromotionPopupBottomWrapper>
              <StyledSearchPromotionPopupBottom>
                {getLocale('searchPromotionNTPPopupBottom')}
              </StyledSearchPromotionPopupBottom>
              <Arrow />
            </StyledSearchPromotionPopupBottomWrapper>
          </StyledSearchPromotionPopup>
          <SearchBox onSubmit={this.props.onTryBraveSearch} />
        </StyledSearchPromotion>
      </StyledSearchPromotionWrapper>
    )
  }
}
