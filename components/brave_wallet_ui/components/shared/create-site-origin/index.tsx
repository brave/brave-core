import * as React from 'react'
import { OriginInfo } from '../../../constants/types'

export interface Props {
  originInfo: OriginInfo
}

const CreateSiteOrigin = (props: Props) => {
  const { originInfo } = props

  const { eTldPlusOne, origin } = originInfo

  const url = React.useMemo(() => {
    if (eTldPlusOne) {
      const before = origin.split(eTldPlusOne)[0]
      const after = origin.split(eTldPlusOne)[1]
      // Will inherit styling from parent container
      return <span>{before}<b>{eTldPlusOne}</b>{after}</span>
    }
    return <span>{origin}</span>
  }, [eTldPlusOne, origin])

  return (<>{url}</>)
}
export default CreateSiteOrigin
