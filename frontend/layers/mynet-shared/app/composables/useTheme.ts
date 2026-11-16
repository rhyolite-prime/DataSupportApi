export function useTheme() {
  const isDark = useState<boolean>('mynet-theme-dark', () => false)

  const applyTheme = (dark: boolean) => {
    if (!import.meta.client) return
    document.documentElement.classList.toggle('dark', dark)
    localStorage.setItem('mynet-theme', dark ? 'dark' : 'light')
  }

  const initializeTheme = () => {
    if (!import.meta.client) return
    const stored = localStorage.getItem('mynet-theme')
    const dark = stored === 'dark'
    isDark.value = dark
    applyTheme(dark)
  }

  const toggleTheme = () => {
    isDark.value = !isDark.value
    applyTheme(isDark.value)
  }

  return { isDark, initializeTheme, toggleTheme }
}
