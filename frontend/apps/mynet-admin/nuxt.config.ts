// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
  compatibilityDate: '2026-09-01',
  devtools: { enabled: false },
  future: { compatibilityVersion: 4 },

  // Consume the shared MyNet Nuxt layer via relative path (portable for standalone builds)
  extends: ['../../layers/mynet-shared'],

  devServer: {
    host: '0.0.0.0',
    port: 3100,
  },

  vite: {
    server: {
      allowedHosts: true,
    },
  },

  runtimeConfig: {
    magmaApiBaseUrl: process.env.NUXT_MAGMA_API_BASE_URL || '',
    public: {
      portalKind: 'admin',
      mockMode: true,
    },
  },

  app: {
    head: {
      titleTemplate: '%s · MyNet Admin',
      title: 'MyNet Admin & Network Control',
      meta: [
        {
          name: 'description',
          content: 'MyNet Broadband Admin & NOC Control Plane — Zones, MikroTik RouterOS fleet, FreeRADIUS subscribers, vouchers, agents, and revenue.',
        },
        { name: 'theme-color', content: '#1E40AF' },
      ],
    },
  },

  typescript: {
    strict: true,
  },
})
