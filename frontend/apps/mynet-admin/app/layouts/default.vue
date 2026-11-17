<script setup lang="ts">
const route = useRoute()
const mobileOpen = ref(false)
const { initBrand } = useBrand()
const { isDark, initializeTheme, toggleTheme } = useTheme()
const { session, refreshSession, logout, resetDemoData } = usePortalSession('admin')
const api = useMyNetApi()
const toast = useToast()

initBrand()

const openAlertCount = useState<number>('mynet-admin-alert-count', () => 0)

async function loadAlertBadge() {
  try {
    const overview = await api.get<any>('/admin/overview')
    openAlertCount.value = overview?.kpis?.openAlerts ?? 0
  } catch {
    // ignore badge fetch errors
  }
}

await refreshSession()
if (!session.value && route.path !== '/login') {
  await navigateTo('/login')
}

const pageTitle = computed(() => adminPageTitle(route.path))

async function handleResetDemo() {
  await resetDemoData()
  await refreshSession()
  await loadAlertBadge()
  toast.success('Demo dataset reset', 'All MyNet zones, vouchers, wallets, and sessions restored to initial state.')
  reloadNuxtApp()
}

onMounted(() => {
  initializeTheme()
  void loadAlertBadge()
})
</script>

<template>
  <div class="min-h-screen bg-canvas text-ink">
    <PortalSidebar
      :mobile-open="mobileOpen"
      workspace-label="Network Control Plane"
      dashboard-to="/"
      :navigation-groups="adminNavigationGroups"
      :user="session"
      footer-subtitle="MyNet Admin Portal"
      @close="mobileOpen = false"
      @logout="logout"
    />

    <div class="min-h-screen transition-[padding] duration-200 lg:pl-[264px]">
      <PortalTopbar
        :dark="isDark"
        workspace-label="MyNet Admin"
        :page-title="pageTitle"
        search-placeholder="Search zones, MikroTik devices, subscribers, vouchers…"
        :search-items="adminSearchItems"
        :user="session"
        :alert-count="openAlertCount"
        alerts-to="/monitoring"
        @toggle-menu="mobileOpen = true"
        @toggle-theme="toggleTheme"
        @logout="logout"
        @reset-demo="handleResetDemo"
      />

      <!-- Context Status Strip (inspired by BusinessDesk workspace quick bar) -->
      <div class="no-print mx-auto flex w-full max-w-[1600px] flex-wrap items-center justify-between gap-3 px-4 pt-5 sm:px-6 lg:px-9">
        <div class="flex flex-wrap items-center gap-2 text-xs text-ink-faint">
          <span class="inline-flex items-center gap-1.5 rounded-lg border border-line bg-surface px-2.5 py-1 font-semibold text-ink-soft">
            <AppIcon name="globe-alt" :size="14" class="text-primary" />
            Tenant: MyNet Broadband Ghana
          </span>
          <span class="inline-flex items-center gap-1.5 rounded-lg border border-line bg-surface px-2.5 py-1">
            WireGuard Hub: <code class="font-mono text-[11px] font-semibold text-ink">10.200.0.1/24</code>
          </span>
          <span class="hidden items-center gap-1.5 rounded-lg border border-line bg-surface px-2.5 py-1 md:inline-flex">
            FreeRADIUS: <strong class="text-emerald-600 dark:text-emerald-400">Primary + Standby Online</strong>
          </span>
        </div>

        <div class="flex flex-wrap items-center gap-2 text-xs">
          <NuxtLink
            to="/vouchers"
            class="rounded-lg border border-line bg-surface px-3 py-1.5 font-semibold text-ink-soft transition hover:border-primary/40 hover:text-primary"
          >
            + Issue voucher batch
          </NuxtLink>
          <NuxtLink
            to="/subscribers"
            class="rounded-lg border border-line bg-surface px-3 py-1.5 font-semibold text-ink-soft transition hover:border-primary/40 hover:text-primary"
          >
            Subscriber lookup
          </NuxtLink>
          <NuxtLink
            to="/devices"
            class="rounded-lg border border-line bg-surface px-3 py-1.5 font-semibold text-ink-soft transition hover:border-primary/40 hover:text-primary"
          >
            RouterOS fleet
          </NuxtLink>
        </div>
      </div>

      <main class="mx-auto w-full max-w-[1600px] px-4 py-6 sm:px-6 lg:px-9">
        <slot />
      </main>

      <ToastHost />

      <footer class="no-print px-4 pt-2 pb-6 text-center text-[11px] text-ink-faint sm:px-6 lg:px-9">
        MyNet Control Plane · Powered by Magmanet
      </footer>
    </div>
  </div>
</template>
