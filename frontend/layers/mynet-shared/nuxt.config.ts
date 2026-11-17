import { fileURLToPath } from 'node:url'
import tailwindcss from '@tailwindcss/vite'

/**
 * Shared MyNet Nuxt 4 Layer
 * ─────────────────────────
 * Inherited by both standalone deployable applications:
 *  - apps/mynet-admin (MyNet Admin & Network Control Plane Portal)
 *  - apps/mynet-agent (MyNet Reseller & Field Agent Portal)
 *
 * Owns:
 *  - Tailwind v4 + white-label CSS custom property design tokens
 *  - Self-hosted Sora variable font
 *  - UI component primitives (BaseCard, StatCard, DataTable, ModalShell, etc.)
 *  - Shared shell chrome (PortalSidebar, PortalTopbar,PortalAuthLayout)
 *  - Shared domain types, formatters, branding presets & API composable
 *  - Stateful Nitro mock engine + MagmaNet DataSupportApi domain store
 */
const layerApp = fileURLToPath(new URL('./app', import.meta.url))
const layerServer = fileURLToPath(new URL('./server', import.meta.url))

export default defineNuxtConfig({
  compatibilityDate: '2026-09-01',
  future: { compatibilityVersion: 4 },

  vite: {
    plugins: [tailwindcss()],
    optimizeDeps: {
      include: [
        '@lucide/vue',
        'echarts/charts',
        'echarts/components',
        'echarts/core',
        'echarts/renderers',
      ],
    },
  },

  css: [fileURLToPath(new URL('./app/assets/css/main.css', import.meta.url))],

  components: [
    {
      path: fileURLToPath(new URL('./app/components', import.meta.url)),
      pathPrefix: false,
    },
  ],

  imports: {
    dirs: [
      fileURLToPath(new URL('./app/composables', import.meta.url)),
      fileURLToPath(new URL('./app/utils', import.meta.url)),
    ],
  },

  alias: {
    '@mynet-shared': layerApp,
    '@mynet-server': layerServer,
  },
  nitro: {
    alias: {
      '@mynet-shared': layerApp,
      '@mynet-server': layerServer,
    },
  },

  app: {
    head: {
      htmlAttrs: { lang: 'en' },
      meta: [
        { charset: 'utf-8' },
        { name: 'viewport', content: 'width=device-width, initial-scale=1' },
      ],
    },
  },
})
