<script setup lang="ts">
const route = useRoute()
const mobileOpen = ref(false)
const { initBrand } = useBrand()
const { isDark, initializeTheme, toggleTheme } = useTheme()
const { session, refreshSession, logout, resetDemoData } = usePortalSession('agent')
const { queue, simulateOffline, loadQueue, syncAll } = useOfflineSalesQueue()
const api = useMyNetApi()
const toast = useToast()

initBrand()

const agentFloat = useState<number>('mynet-agent-float-badge', () => 0)
const agentTier = useState<string>('mynet-agent-tier-badge', () => 'PREMIER')
const agentTerritory = useState<string>('mynet-agent-zone-badge', () => 'Accra')

async function refreshWalletStrip() {
  try {
    const dash = await api.get<any>('/agent/dashboard')
    if (dash?.agent) {
      agentFloat.value = dash.agent.floatBalance
      agentTier.value = dash.agent.tier
      agentTerritory.value = dash.agent.territoryZoneName
    }
  } catch {
    // ignore transient errors
  }
}

await refreshSession()
if (!session.value && route.path !== '/login') {
  await navigateTo('/login')
}

const pageTitle = computed(() => agentPageTitle(route.path))

async function handleResetDemo() {
  await resetDemoData()
  await refreshSession()
  await refreshWalletStrip()
  toast.success('Demo dataset reset', 'Reseller wallet, voucher stock, and sales history restored.')
  reloadNuxtApp()
}

async function handleSyncQueue() {
  const res = await syncAll(api)
  await refreshWalletStrip()
  if (res.syncedCount > 0) {
    toast.success('Offline sales synced', `${res.syncedCount} queued voucher sale(s) settled to MyNet ledger.`)
  }
}

onMounted(() => {
  initializeTheme()
  loadQueue()
  void refreshWalletStrip()
})
</script>

<template>
  <div class="min-h-screen bg-canvas text-ink">
    <PortalSidebar
      :mobile-open="mobileOpen"
      workspace-label="Reseller & Voucher POS"
      dashboard-to="/"
      :navigation-groups="agentNavigationGroups"
      :user="session"
      footer-subtitle="MyNet Reseller POS · Field Agent App"
      @close="mobileOpen = false"
      @logout="logout"
    />

    <div class="min-h-screen transition-[padding] duration-200 lg:pl-[264px]">
      <PortalTopbar
        :dark="isDark"
        workspace-label="MyNet Agent"
        :page-title="pageTitle"
        search-placeholder="Jump to voucher POS, subscriber top-up, float wallet…"
        :search-items="agentSearchItems"
        :user="session"
        @toggle-menu="mobileOpen = true"
        @toggle-theme="toggleTheme"
        @logout="logout"
        @reset-demo="handleResetDemo"
      />

      <!-- Reseller Float & Connectivity Context Strip -->
      <div class="no-print mx-auto flex w-full max-w-[1600px] flex-wrap items-center justify-between gap-3 px-4 pt-5 sm:px-6 lg:px-9">
        <div class="flex flex-wrap items-center gap-2 text-xs">
          <NuxtLink
            to="/wallet"
            class="inline-flex items-center gap-1.5 rounded-lg border border-line bg-surface px-3 py-1.5 font-bold text-ink shadow-2xs tnum hover:border-primary/40"
          >
            <AppIcon name="wallet" :size="15" class="text-primary" />
            Float: <span class="text-primary">{{ formatCurrency(agentFloat) }}</span>
          </NuxtLink>
          <span class="inline-flex items-center gap-1.5 rounded-lg border border-line bg-surface px-2.5 py-1.5 text-ink-soft">
            <AppIcon name="map-pin" :size="14" class="text-amber-500" />
            Territory: <strong>{{ agentTerritory }}</strong>
          </span>
          <Badge tone="purple" size="sm">{{ agentTier }} TIER</Badge>
        </div>

        <div class="flex flex-wrap items-center gap-2 text-xs">
          <button
            type="button"
            class="inline-flex items-center gap-1.5 rounded-lg border px-2.5 py-1.5 font-semibold transition"
            :class="simulateOffline ? 'border-amber-400 bg-amber-50 text-amber-800 dark:bg-amber-950/60 dark:text-amber-200' : 'border-line bg-surface text-ink-soft hover:text-ink'"
            @click="simulateOffline = !simulateOffline"
          >
            <span class="h-2 w-2 rounded-full" :class="simulateOffline ? 'bg-amber-500' : 'bg-emerald-500'" />
            {{ simulateOffline ? 'Field Offline Queue Active' : 'Online · Instant Issue' }}
          </button>

          <BaseButton
            v-if="queue.length"
            size="sm"
            variant="accent"
            icon="arrow-path"
            @click="handleSyncQueue"
          >
            Sync {{ queue.length }} Queued
          </BaseButton>

          <NuxtLink
            to="/sell"
            class="rounded-lg bg-primary px-3 py-1.5 font-semibold text-on-primary shadow-xs transition hover:bg-primary-dark"
          >
            + Sell Voucher PIN
          </NuxtLink>
        </div>
      </div>

      <main class="mx-auto w-full max-w-[1600px] px-4 py-6 sm:px-6 lg:px-9">
        <slot />
      </main>

      <ToastHost />

      <footer class="no-print px-4 pt-2 pb-6 text-center text-[11px] text-ink-faint sm:px-6 lg:px-9">
        MyNet Broadband Reseller Network · Powered by RhyolitePrime
      </footer>
    </div>
  </div>
</template>
