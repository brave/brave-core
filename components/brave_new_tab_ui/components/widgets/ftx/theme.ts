import IBraveTheme from 'brave-ui/theme/theme-interface'

export default function customizeTheme (theme: IBraveTheme): IBraveTheme {
  return {
    ...theme,
    color: {
      ...theme.color,
      primaryBackground: "rgba(2, 166, 194, 1)",
      secondaryBackground: "rgba(33, 46, 60, 1)"
    }
  }
}