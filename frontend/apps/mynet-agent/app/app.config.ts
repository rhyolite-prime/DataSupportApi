export default defineAppConfig({
  brand: {
    productName: 'Reseller & Voucher POS',
    portalKind: 'agent' as const,
    institutionName: 'MyNet Broadband',
    institutionShort: 'MyNet',
    tagline: 'Instant WiFi Vouchers, Subscriber Top-Up & MoMo Commissions',
    theme: {
      primary: '#4338CA',
      primaryDark: '#3730A3',
      primaryLight: '#EEF2FF',
      onPrimary: '#FFFFFF',
      sidebar: '#13112E',
      accent: '#F97316',
    },
    currency: { code: 'GHS', symbol: 'GH₵', locale: 'en-GH' },
    contact: { email: 'agents@mynet.gh', phone: '+233 30 274 8810' },
    footerNote: 'MyNet Reseller POS · Powered by RhyolitePrime',
  },
})
