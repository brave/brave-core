import * as React from 'react'

import {
  LabelWrapper,
  ChartLabel
} from '../style'

function CustomTooltip (props: any) {
  const parseDate = (date: Date) => {
    const formatedDate = new Date(date).toLocaleDateString('en-US', { month: 'numeric', day: 'numeric', year: 'numeric' })
    const formatedTime = new Date(date).toLocaleTimeString('en-US', { hour: 'numeric', minute: 'numeric', hour12: true })
    return `${formatedDate} ${formatedTime}`
  }

  if (props.active && props.payload && props.payload.length) {

    React.useLayoutEffect(() => {
      props.onUpdatePosition(props.coordinate.x)
      props.onUpdateBalance(props.payload[0].value)
    }, [props])

    const xLeftCoordinate = Math.trunc(props.coordinate.x)
    const viewBoxWidth = Math.trunc(props.viewBox.width)
    const xRightCoordinate = xLeftCoordinate - viewBoxWidth
    const isEndOrMiddle = xRightCoordinate >= -46 ? 'end' : 'middle'
    const labelPosition = xLeftCoordinate <= 62 ? 'start' : isEndOrMiddle
    const middleEndTranslate = xRightCoordinate >= 8 ? 0 : Math.abs(xRightCoordinate) + 8

    return (
      <LabelWrapper labelTranslate={labelPosition === 'start' ? xLeftCoordinate : middleEndTranslate} labelPosition={labelPosition}>
        <ChartLabel>{parseDate(props.label)}</ChartLabel>
      </LabelWrapper>
    )
  } else {
    return null
  }
}

export default CustomTooltip
