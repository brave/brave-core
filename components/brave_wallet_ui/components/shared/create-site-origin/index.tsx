import * as React from 'react'

export interface Props {
  originSpec: string
  eTldPlusOne: string
}

const CreateSiteOrigin = (props: Props) => {
  const { originSpec, eTldPlusOne } = props

  const url = React.useMemo(() => {
    if (eTldPlusOne) {
      const before = originSpec.split(eTldPlusOne)[0]
      const after = originSpec.split(eTldPlusOne)[1]
      // Will inherit styling from parent container
      return <span>{before}<b>{eTldPlusOne}</b>{after}</span>
    }
    return <span>{originSpec}</span>
  }, [eTldPlusOne, originSpec])

  return (<>{url}</>)
}
export default CreateSiteOrigin
