// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
  compatibilityDate: '2026-09-01',
  devtools: { enabled: false },
  future: { compatibilityVersion: 4 },

  // Consume the shared MyNet Nuxt layer via relative path (portable for standalone builds)
  extends: ['../../layers/mynet-shared'],

  devServer: {
    host: '0.0.0.0',
    port: 3101,
  },

  vite: {
    server: {
      allowedHosts: true,
    },
  },

  runtimeConfig: {
    magmaApiBaseUrl: process.env.NUXT_MAGMA_API_BASE_URL || '',
    public: {
      portalKind: 'agent',
      mockMode: true,
    },
  },

  app: {
    head: {
      titleTemplate: '%s · MyNet Agent POS',
      title: 'MyNet Reseller & Voucher POS',
      meta: [
        {
          name: 'description',
          content: 'MyNet Broadband Reseller & Field Agent POS — Instant WiFi voucher PINs, QR receipts, subscriber top-up, stock batches, and MoMo float settlements.',
        },
        { name: 'theme-color', content: '#4338CA' },
      ],
    },
  },

  typescript: {
    strict: true,
  },
})
