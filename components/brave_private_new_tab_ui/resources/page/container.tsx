/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import styled from 'styled-components'
import getPageHandlerInstance from './api/brave_page_handler'
import { loadTimeData } from '../../../common/loadTimeData'

import BackgroundView from './components/background-view'
import SearchBox from './components/search-box'
import DisclaimerDialog from './components/disclaimer-dialog'
import BadgeTor from './components/badge-tor'
import { useTorObserver, useHasDisclaimerDismissed } from './hooks'

const CenterView = styled.div`
  width: 100%;
  max-width: 640px;
  padding: 0 24px 120px 24px;
`

const DialogBox = styled.div`
  position: absolute;
  left: 24px;
  bottom: 24px;
`

const BadgeBox = styled(DialogBox)`
  bottom: unset;
  top: 24px;
`

const BadgePrivateWindow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  background: rgba(255, 255, 255, 0.15);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  color: white;
  font-weight: 600;
  font-size: 14px;
  border-radius: 4px;
  padding: 8px 16px;

  svg {
    margin-right: 12px;
  }
`

function Container () {
  const isWindowTor = loadTimeData.getBoolean('isWindowTor')

  const { isConnected, progress, message, connectionStatus } = useTorObserver()
  const { hasDisclaimerDismissed } = useHasDisclaimerDismissed()

  let badgeElement = (
    <BadgePrivateWindow>
      {/* <svg width="20" height="8" fill="none" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M1.095 3.036c.141.46.424 3.121 1.273 3.889.874.79 4.173.778 4.805.436C8.587 6.595 9.16 4.119 9.41 3.035c.14-.614.59-.614.59-.614s.474 0 .616.613c.25 1.085.798 3.568 2.21 4.333.632.343 3.931.355 4.807-.435.847-.768 1.13-3.437 1.271-3.898.14-.46.848-.92.99-1.073.142-.154.142-.767 0-.921-.283-.307-3.618-.58-7.21-.154-.716.086-.989.307-2.685.307-1.696 0-1.97-.222-2.686-.307C3.725.46.39.734.106 1.04c-.141.154-.141.768 0 .921.141.154.848.614.989 1.075Z" fill="currentColor"/></svg> */}
      {getLocale('headerTitle')}
    </BadgePrivateWindow>
  )

  let dialogContent = (
    <p>
      {getLocale('headerText1')}
      {' '}
      {getLocale('headerText2')}
    </p>
  )

  if (isWindowTor) {
    badgeElement = (
      <BadgeTor
        isConnected={isConnected}
        progress={progress ?? ''}
        message={message ?? ''}
        connectionStatus={connectionStatus}
      />
    )

    dialogContent = (
      <>
        <p>{getLocale('headerTorText')}</p>
      </>
    )
  }

  const handleSearchSubmit = (value: string, openNewTab: boolean) => {
    getPageHandlerInstance().pageHandler.goToBraveSearch(value, openNewTab)
  }

  const handleDialogClose = () => {
    if (hasDisclaimerDismissed) return
    getPageHandlerInstance().pageHandler.setDisclaimerDismissed(true)
  }

  return (
    <BackgroundView isTor={isWindowTor}>
      <BadgeBox>{badgeElement}</BadgeBox>
      <CenterView>
        <SearchBox onSubmit={handleSearchSubmit} />
      </CenterView>
      <DialogBox>
        {(hasDisclaimerDismissed != null) && (
          <DisclaimerDialog
            title={isWindowTor ? getLocale('headerTorTitle') : getLocale('headerTitle')}
            isOpen={!hasDisclaimerDismissed}
            onClose={handleDialogClose}
          >
            {dialogContent}
          </DisclaimerDialog>
        )}
      </DialogBox>
    </BackgroundView>
  )
}

export default Container
