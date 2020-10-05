function getCryptoDotComTickerInfo (asset: string) {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getTickerInfo(`${asset}_USDT`, (resp: any) => {
      resolve({ [asset]: resp })
    })
  })
}

function getCryptoDotComAssetRankings () {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getAssetRankings((resp: any) => {
      resolve(resp)
    })
  })
}

function getCryptoDotComChartData (asset: string) {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getChartData(`${asset}_USDT`, (resp: any) => {
      resolve({ [asset]: resp })
    })
  })
}

function getCryptoDotComSupportedPairs () {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getSupportedPairs((resp: any) => {
      resolve(resp)
    })
  })
}

export async function fetchCryptoDotComTickerPrices (assets: string[]) {
  const assetReqs = assets.map(asset => getCryptoDotComTickerInfo(asset))
  const assetResps = await Promise.all(assetReqs).then((resps: object[]) => resps)
  return assetResps.reduce((all, current) => ({ ...current, ...all }), {})
}

export async function fetchCryptoDotComLosersGainers () {
  return getCryptoDotComAssetRankings().then((resp: any) => resp)
}

export async function fetchCryptoDotComCharts (assets: string[]) {
  const chartReqs = assets.map(asset => getCryptoDotComChartData(asset))
  const chartResps = await Promise.all(chartReqs).then((resps: object[]) => resps)
  return chartResps.reduce((all, current) => ({ ...current, ...all }), {})
}

export async function fetchCryptoDotComSupportedPairs () {
  return getCryptoDotComSupportedPairs().then((resp: any) => resp)
}
