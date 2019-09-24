import sendBackgroundMessage from './send-background-message'

export default async function PerformBypassInBackgroundScript (bypassArgs) {
  const didBypass = await sendBackgroundMessage({
    type: 'perform-bypass',
    bypassArgs
  })
  return didBypass
}