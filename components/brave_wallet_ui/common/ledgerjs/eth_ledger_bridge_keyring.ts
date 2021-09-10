const { EventEmitter } = require('events')

import { kLedgerKeyringType } from '../../constants/types'

export default class LedgerBridgeKeyring extends EventEmitter {
  constructor () {
    super()
  }
  type = () => {
    return kLedgerKeyringType
  }
  getDefaultKeyringInfo = () => {
    return new Promise((resolve, reject) => {
      resolve({
        isDefaultKeyringCreated: false,
        isLocked: false,
        isBackedUp: false,
        accountInfos: []
      })
    })
  }
}
