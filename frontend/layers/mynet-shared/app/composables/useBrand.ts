/**
 * White-label branding & live theme engine for MyNet portals.
 * Uses distinct telecom / broadband themes (NOT BusinessDesk's forest green/gold):
 *  - MyNet Cobalt & Cyan (default Admin NOC palette)
 *  - MyNet Indigo & Coral (default Agent POS palette)
 *  - MyNet Plum & Magenta (Enterprise ISP preset)
 *  - MyNet Slate & Sky (High-contrast Field preset)
 */
export interface BrandTheme {
  primary: string
  primaryDark: string
  primaryLight: string
  onPrimary: string
  sidebar: string
  accent: string
}

export interface BrandPreset {
  key: string
  label: string
  theme: BrandTheme
}

export const MYNET_BRAND_PRESETS: BrandPreset[] = [
  {
    key: 'mynet-cobalt',
    label: 'MyNet · Sapphire & Cyan',
    theme: {
      primary: '#1E40AF',
      primaryDark: '#1E3A8A',
      primaryLight: '#EFF6FF',
      onPrimary: '#FFFFFF',
      sidebar: '#0B132B',
      accent: '#06B6D4',
    },
  },
  {
    key: 'mynet-indigo-coral',
    label: 'MyNet · Indigo & Coral',
    theme: {
      primary: '#4338CA',
      primaryDark: '#3730A3',
      primaryLight: '#EEF2FF',
      onPrimary: '#FFFFFF',
      sidebar: '#13112E',
      accent: '#F97316',
    },
  },
  {
    key: 'mynet-crimson-violet',
    label: 'MyNet · Mulberry & Rose',
    theme: {
      primary: '#9333EA',
      primaryDark: '#7E22CE',
      primaryLight: '#FAF5FF',
      onPrimary: '#FFFFFF',
      sidebar: '#1E1033',
      accent: '#EC4899',
    },
  },
  {
    key: 'mynet-ocean-sky',
    label: 'MyNet · Atlantic & Sky',
    theme: {
      primary: '#0369A1',
      primaryDark: '#075985',
      primaryLight: '#F0F9FF',
      onPrimary: '#FFFFFF',
      sidebar: '#081C2C',
      accent: '#38BDF8',
    },
  },
]

export function useBrand() {
  const appConfig = useAppConfig()
  const brand = computed(() => appConfig.brand)
  const activePreset = useState<string>('mynet-brand-preset', () => 'default')
  const customTheme = useState<BrandTheme | null>('mynet-custom-theme', () => null)

  const currentTheme = computed<BrandTheme>(() => customTheme.value ?? (brand.value.theme as BrandTheme))

  const varsFor = (theme: BrandTheme) => ({
    '--brand-primary': theme.primary,
    '--brand-primary-dark': theme.primaryDark,
    '--brand-primary-light': theme.primaryLight,
    '--brand-on-primary': theme.onPrimary,
    '--brand-sidebar': theme.sidebar,
    '--brand-accent': theme.accent,
  })

  function applyToDocument(theme: BrandTheme) {
    if (!import.meta.client) return
    const root = document.documentElement
    for (const [key, value] of Object.entries(varsFor(theme))) {
      root.style.setProperty(key, value)
    }
  }

  function initBrand() {
    const vars = varsFor(currentTheme.value)
    const css = `:root{${Object.entries(vars).map(([k, v]) => `${k}:${v}`).join(';')}}`
    useHead({
      style: [{ innerHTML: css, id: 'mynet-brand-theme' }],
    })
    if (import.meta.client) {
      const storageKey = `mynet-preset-${brand.value.portalKind}`
      const savedKey = localStorage.getItem(storageKey)
      if (savedKey) {
        const found = MYNET_BRAND_PRESETS.find(p => p.key === savedKey)
        if (found) {
          activePreset.value = found.key
          customTheme.value = found.theme
          applyToDocument(found.theme)
          return
        }
      }
      applyToDocument(currentTheme.value)
    }
  }

  function applyPreset(preset: BrandPreset) {
    activePreset.value = preset.key
    customTheme.value = preset.theme
    applyToDocument(preset.theme)
    if (import.meta.client) {
      localStorage.setItem(`mynet-preset-${brand.value.portalKind}`, preset.key)
    }
  }

  return {
    brand,
    currentTheme,
    presets: MYNET_BRAND_PRESETS,
    activePreset,
    initBrand,
    applyPreset,
  }
}
