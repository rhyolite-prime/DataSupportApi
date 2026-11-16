export default defineAppConfig({
  brand: {
    productName: 'Network Control Plane',
    portalKind: 'admin' as const,
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
    contact: { email: 'noc@mynet.gh', phone: '+233 30 274 8800' },
    footerNote: 'MyNet Admin & NOC Workspace · Powered by RhyolitePrime',
  },
})
