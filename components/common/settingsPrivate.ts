export function getPreference (key: string) {
  return new Promise<chrome.settingsPrivate.PrefObject>(function (resolve) {
    chrome.settingsPrivate.getPref(key, pref => resolve(pref))
  })
}

export function setPreference (key: string, value: any) {
  return new Promise<boolean>(function (resolve) {
    chrome.settingsPrivate.setPref(key, value, null, resolve)
  })
}
