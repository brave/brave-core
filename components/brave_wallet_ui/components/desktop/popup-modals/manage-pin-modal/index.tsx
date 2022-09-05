
// import { DividerLine } from '../../../extension'
import { PageState } from '../../../../constants/types'
import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory } from 'react-router'
import PopupModal from '..'

// utils

// components
// import {
//   PopupModal
// } from '../..'
// import { getLocale } from '$web-common/locale'

// Styled Components
import {
  PinStatusTitle,
} from './style'
import Button from '$web-components/button'
import { WalletActions } from '../../../../common/actions'

// hooks

export const ManagePinModalView = () => {
  const history = useHistory()
  const dispatch = useDispatch()

  const {
    selectedAsset,
    pinStatusOverview,
  } = useSelector(({ page }: { page: PageState }) => page)

  const onCloseMethod = () => {
    history.goBack()
  };

  const onClickPinButton = () => {
    if (selectedAsset)
      dispatch(WalletActions.pinTokenLocaly(selectedAsset))
  }

  const onClickUnpinButton = () => {
    if (selectedAsset)
      dispatch(WalletActions.pinTokenLocaly(selectedAsset))
  }

  console.error('XXXZZZ ' + Object.keys(pinStatusOverview?.remotes))
  return (
    <PopupModal
      title={"Pin status " + (selectedAsset?.name || "")}
      onClose={onCloseMethod}>
      {Array.from(Object.keys(pinStatusOverview?.remotes)).map((serviceName: string) => {
        return (
          <div>
            <PinStatusTitle>{"Pinned remotely " + serviceName + " " + pinStatusOverview?.remotes[serviceName]}</PinStatusTitle>
            <Button
              onClick={onClickPinButton}>
              "Pin"
            </Button>
            <Button
              onClick={onClickUnpinButton}>
              "Unpin"
            </Button>
          </div>
        )
      })
      }



      <PinStatusTitle>{"Pinned localy " + pinStatusOverview?.local}</PinStatusTitle>
    </PopupModal>
  )
}

export default ManagePinModalView
