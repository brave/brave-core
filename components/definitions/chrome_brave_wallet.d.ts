declare namespace chrome.braveWallet {
  const ready: () => void
  const shouldPromptForSetup: (callback: (shouldPrompt: boolean) => void) => void
  const loadUI: (callback: () => void) => void
  const isNativeWalletEnabled: (callback: (enabled: boolean) => void) => void
  const notifyWalletUnlock: () => void
}
