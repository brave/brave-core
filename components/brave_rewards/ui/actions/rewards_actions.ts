/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constant
import { types } from '../constants/rewards_types'

export const createWallet = () => action(types.CREATE_WALLET)

export const onWalletCreated = () => action(types.WALLET_CREATED)

export const onWalletCreateFailed = () => action(types.WALLET_CREATE_FAILED)

export const onSettingSave = (key: string, value: any) => action(types.ON_SETTING_SAVE, {
  key,
  value
})

export const getWalletProperties = () => action(types.GET_WALLET_PROPERTIES)

export const onWalletProperties = (properties: {status: number, wallet: Rewards.WalletProperties}) => action(types.ON_WALLET_PROPERTIES, {
  properties
})

export const getGrant = () => action(types.GET_GRANT)

export const onGrant = (properties: Rewards.Grant) => action(types.ON_GRANT, {
  properties
})

export const getGrantCaptcha = () => action(types.GET_GRANT_CAPTCHA)

export const onGrantCaptcha = (image: string) => action(types.ON_GRANT_CAPTCHA, {
  image
})

export const solveGrantCaptcha = (x: number, y: number) => action(types.SOLVE_GRANT_CAPTCHA, {
  x,
  y
})

export const onGrantFinish = (properties: Rewards.GrantFinish) => action(types.ON_GRANT_FINISH, {
  properties
})

export const onResetGrant = () => action(types.ON_GRANT_RESET)

export const onDeleteGrant = () => action(types.ON_GRANT_DELETE)

export const getWalletPassphrase = () => action(types.GET_WALLLET_PASSPHRASE)

export const onWalletPassphrase = (pass: string) => action(types.ON_WALLLET_PASSPHRASE, {
  pass
})

export const recoverWallet = (key: string) => action(types.RECOVER_WALLET, {
  key
})

export const onRecoverWalletData = (properties: Rewards.RecoverWallet) => action(types.ON_RECOVER_WALLET_DATA, {
  properties
})

export const onModalBackupClose = () => action(types.ON_MODAL_BACKUP_CLOSE)

export const onModalBackupOpen = () => action(types.ON_MODAL_BACKUP_OPEN)

export const onClearAlert = (property: string) => action(types.ON_CLEAR_ALERT, {
  property
})

export const onReconcileStamp = (stamp: number) => action(types.ON_RECONCILE_STAMP, {
  stamp
})

export const onAddresses = (addresses: Record<string, string>) => action(types.ON_ADDRESSES, {
  addresses
})

export const onQRGenerated = (type: Rewards.AddressesType, image: string) => action(types.ON_QR_GENERATED, {
  type,
  image
})

export const onContributeList = (list: Rewards.Publisher[]) => action(types.ON_CONTRIBUTE_LIST, {
  list
})

export const onBalanceReports = (reports: Record<string, Rewards.Report>) => action(types.ON_BALANCE_REPORTS, {
  reports
})
