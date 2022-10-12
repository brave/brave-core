import * as React from 'react'
import styled from 'styled-components'
import { loadTimeData } from '../../../../../common/loadTimeData'
import Configure from './Configure'
import { useBraveNews } from './Context'

const Dialog = styled.dialog`
  border-radius: 8px;
  border: none;
  width: min(100vw, 1092px);
  height: min(100vh, 712px);
  z-index: 1000;
  background: white;
  overflow: hidden;
  padding: 0;
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
`

const shouldRender = loadTimeData.getBoolean('featureFlagBraveNewsV2Enabled')
export default function BraveNewsModal () {
  const { customizePage: page } = useBraveNews()
  const dialogRef = React.useRef<HTMLDialogElement & { showModal: () => void, close: () => void, open: boolean }>()

  // Note: There's no attribute for open modal, so we need
  // to do this instead.
  React.useEffect(() => {
    if (dialogRef.current?.open && !page) { dialogRef.current?.close?.() }
    if (!dialogRef.current?.open && page) { dialogRef.current?.showModal?.() }
  }, [page])

  // Don't render the dialog at all unless Brave News V2 is enabled - no point
  // adding all the extra elements to the DOM if we aren't going to use them.
  return shouldRender ? <Dialog ref={dialogRef as any}>
    <Configure />
  </Dialog> : null
}
