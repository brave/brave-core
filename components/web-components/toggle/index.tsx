import * as React from 'react'

import * as S from './style'

export type Size = 'sm' | 'default'
export type Brand = 'vpn' | 'shields'

interface ToggleProps {
  size?: Size
  isOn?: boolean
  disabled?: boolean
  accessibleLabel?: string
  brand?: Brand
  onChange?: (isOn: boolean) => unknown
}

// We dont set the size for large toggles as they're always consistent
interface FabulouslyLargeToggleProps extends Omit<ToggleProps, 'size'> {}

function useToggleProps (props: ToggleProps) {
  const [isOn, setIsOn] = React.useState(props.isOn ?? false)

  React.useEffect(() => {
    setIsOn(props.isOn ?? false)
  }, [props.isOn])

  const onToggleClick = () => {
    const activated = !isOn
    setIsOn(activated)
    props.onChange?.(activated)
  }

  return ({
    'onClick': onToggleClick,
    'isOn': isOn,
    'disabled': props.disabled,
    'size': props.size,
    'aria-label': props.accessibleLabel,
    'aria-checked': props.isOn,
    'brand': props.brand
  })
}

function Toggle (props: ToggleProps) {
  const toggleProps = useToggleProps(props)

  return (
    <S.ToggleBox
      {...toggleProps}
    />
  )
}

export function FabulouslyLargeToggle (props: FabulouslyLargeToggleProps) {
  const toggleProps = useToggleProps(props)

  return (
    <S.FLToggleBox
      {...toggleProps}
      brand={props.brand}
    />
  )
}

export default Toggle
