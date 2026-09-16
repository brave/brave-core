module.exports.onRpcRequest = async (request) => {
  const { method, params } = request
  if (method === 'echo') {
    return { echoed: params }
  }
  if (method === 'roundTrip') {
    await snap.request({
      method: 'snap_manageState',
      params: { operation: 'update', newState: { seen: params } },
    })
    const state = await snap.request({
      method: 'snap_manageState',
      params: { operation: 'get' },
    })
    return { fromBrowser: state }
  }
  throw new Error(`unknown method: ${method}`)
}
