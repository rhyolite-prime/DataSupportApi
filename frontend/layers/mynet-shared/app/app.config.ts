/**
 * Default MyNet white-label branding.
 * Each standalone portal (mynet-admin, mynet-agent) overrides productName,
 * tagline, and theme tokens in its own app.config.ts while sharing the UI layer.
 * Note: Commercial product name is MyNet (engineering engine: MagmaNet).
 */
export default defineAppConfig({
  brand: {
    productName: 'MyNet Cloud',
    portalKind: 'admin' as 'admin' | 'agent',
    institutionName: 'MyNet Broadband',
    institutionShort: 'MyNet',
    tagline: 'Community WiFi, Fixed Wireless & Agent Distribution',
    theme: {
      primary: '#1E40AF',
      primaryDark: '#1E3A8A',
      primaryLight: '#EFF6FF',
      onPrimary: '#FFFFFF',
      sidebar: '#0B132B',
      accent: '#06B6D4',
    },
    currency: { code: 'GHS', symbol: 'GH₵', locale: 'en-GH' },
    contact: { email: 'support@mynet.gh', phone: '+233 30 274 8800' },
    footerNote: 'MyNet Broadband Ghana · Powered by RhyolitePrime',
  },
})
