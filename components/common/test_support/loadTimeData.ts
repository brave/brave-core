import { loadTimeData } from '$web-common/loadTimeData'

export const addLocaleStrings = (strings: Record<string, string>) => {
  addLoadTimeData(strings)
}

export const addLoadTimeData = (
  data: Record<string, string | boolean | number>
) => {
  Object.assign(loadTimeData.data_, data)
}
