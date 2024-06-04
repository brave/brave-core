(_ => {
  const W = window
  const C = W.Cookies
  const D = W.document
  const BU = W.BRAVE
  const IDB = W.idbKeyval
  const exceptionEncoding = '*exception*'

  const isImmediateRemoteChildFrame = _ => {
    // If we're the top document, were clearly not a child frame
    if (W.top === W) {
      return false
    }
    // If our parent isn't the top document, we're too nested so can't
    // be the immediate remote child frame.
    if (W.top !== W.parent) {
      return false
    }
    // Last, see if we're remote by seeing if we trigger a SOP violation
    // by reading the location of the parent.
    try {
      if (window.parent.location.href) {}
      return false
    } catch (_) {
      return true
    }
  }

  const clearStorage = async key => {
    const result = Object.create(null)
    try {
      if (W.navigator.cookieEnabled === false || !await W.document.hasStorageAccess()) {
        result.cookies = exceptionEncoding
      } else {
        C.remove(key)
        result.cookies = true
      }
    } catch (_) {
      result.cookies = exceptionEncoding
    }

    try {
      W.localStorage.removeItem(key)
      result['local-storage'] = true
    } catch (_) {
      result['local-storage'] = exceptionEncoding
    }

    try {
      W.sessionStorage.removeItem(key)
      result['session-storage'] = true
    } catch (_) {
      result['session-storage'] = exceptionEncoding
    }

    try {
      await IDB.del(key)
      result['index-db'] = true
    } catch (_) {
      result['index-db'] = exceptionEncoding
    }

    return result
  }

  const readStorageAction = async key => {
    const result = Object.create(null)
    try {
      if (W.navigator.cookieEnabled === false || !await W.document.hasStorageAccess()) {
        result.cookies = exceptionEncoding
      } else {
        const readCookieValue = C.get(key)
        result.cookies = readCookieValue === undefined ? null : readCookieValue
      }
    } catch (_) {
      result.cookies = exceptionEncoding
    }

    try {
      result['local-storage'] = W.localStorage.getItem(key)
    } catch (_) {
      result['local-storage'] = exceptionEncoding
    }

    try {
      result['session-storage'] = W.sessionStorage.getItem(key)
    } catch (_) {
      result['session-storage'] = exceptionEncoding
    }

    try {
      result['index-db'] = await IDB.get(key)
    } catch (_) {
      result['index-db'] = exceptionEncoding
    }

    return result
  }

  const writeStorageAction = async (key, value) => {
    const result = Object.create(null)
    try {
      if (W.navigator.cookieEnabled === false || !await W.document.hasStorageAccess()) {
        result.cookies = false
      } else {
        C.set(key, value, {
          secure: true,
          sameSite: 'None'
        })
        result.cookies = C.get(key) === value
      }
    } catch (_) {
      result.cookies = exceptionEncoding
    }

    try {
      W.localStorage.setItem(key, value)
      result['local-storage'] = true
    } catch (_) {
      result['local-storage'] = exceptionEncoding
    }

    try {
      W.sessionStorage.setItem(key, value)
      result['session-storage'] = true
    } catch (_) {
      result['session-storage'] = exceptionEncoding
    }

    try {
      await IDB.set(key, value)
      result['index-db'] = true
    } catch (_) {
      result['index-db'] = exceptionEncoding
    }

    return result
  }

  let nestedFrame
  if (isImmediateRemoteChildFrame() === true) {
    nestedFrame = D.createElement('iframe')
    D.body.appendChild(nestedFrame)
    nestedFrame.src = BU.otherOriginUrl(W.location.pathname)
  }

  const onMessage = async (action, msg) => {
    switch (action) {
      case 'storage::clear':
        return await clearStorage(msg.key)

      case 'storage::read':
        return await readStorageAction(msg.key)

      case 'storage::write':
        return await writeStorageAction(msg.key, msg.value)

      case 'storage::nested-frame':
        if (nestedFrame === undefined) {
          BU.logger(`unexpected storage::nested-frame: ${W.location.toString()}`)
          return
        }
        return await BU.sendPostMsg(nestedFrame.contentWindow, 'storage::read', {
          key: msg.key
        })
    }
  }
  BU.receivePostMsg(onMessage)
})()
