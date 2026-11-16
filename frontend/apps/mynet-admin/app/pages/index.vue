<script setup lang="ts">
useHead({ title: 'NOC & Commercial Overview' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const loading = ref(false)
const data = ref<any>(null)

async function load() {
  loading.value = true
  try {
    data.value = await api.get('/admin/overview')
  } catch (e: any) {
    toast.error('Failed to load overview', e.message)
  } finally {
    loading.value = false
  }
}

await load()

async function ackAlert(id: string) {
  try {
    await api.post(`/admin/alerts/${id}/ack`)
    toast.success('Alert acknowledged')
    await load()
  } catch (e: any) {
    toast.error('Could not acknowledge alert', e.message)
  }
}

const bandwidthChartOption = computed(() => {
  const rows = data.value?.bandwidthSeries || []
  return {
    tooltip: { trigger: 'axis' },
    legend: { bottom: 0, textStyle: { color: '#64748B', fontSize: 11 } },
    grid: { top: 16, left: 38, right: 16, bottom: 32 },
    xAxis: {
      type: 'category',
      data: rows.map((r: any) => r.hour),
      axisLabel: { color: '#64748B', fontSize: 11 },
    },
    yAxis: {
      type: 'value',
      name: 'Mbps',
      nameTextStyle: { color: '#94A3B8', fontSize: 10 },
      axisLabel: { color: '#64748B', fontSize: 11 },
      splitLine: { lineStyle: { color: 'rgba(148,163,184,0.16)' } },
    },
    series: [
      {
        name: 'Kaneshie Down',
        type: 'line',
        smooth: true,
        data: rows.map((r: any) => r.kaneshieDown),
        itemStyle: { color: '#2563EB' },
        areaStyle: { color: 'rgba(37,99,235,0.12)' },
      },
      {
        name: 'Mamprobi Down',
        type: 'line',
        smooth: true,
        data: rows.map((r: any) => r.mamprobiDown),
        itemStyle: { color: '#06B6D4' },
        areaStyle: { color: 'rgba(6,182,212,0.10)' },
      },
    ],
  }
})

const revenueChartOption = computed(() => {
  const rows = data.value?.revenueSeries || []
  return {
    tooltip: { trigger: 'axis' },
    legend: { bottom: 0, textStyle: { color: '#64748B', fontSize: 11 } },
    grid: { top: 16, left: 44, right: 16, bottom: 32 },
    xAxis: {
      type: 'category',
      data: rows.map((r: any) => r.day),
      axisLabel: { color: '#64748B', fontSize: 11 },
    },
    yAxis: {
      type: 'value',
      axisLabel: { color: '#64748B', fontSize: 11 },
      splitLine: { lineStyle: { color: 'rgba(148,163,184,0.16)' } },
    },
    series: [
      { name: 'Portal MoMo', type: 'bar', stack: 'rev', data: rows.map((r: any) => r.momo), itemStyle: { color: '#1E40AF', borderRadius: [0, 0, 0, 0] } },
      { name: 'Agent Vouchers', type: 'bar', stack: 'rev', data: rows.map((r: any) => r.vouchers), itemStyle: { color: '#06B6D4' } },
      { name: 'Home PPPoE', type: 'bar', stack: 'rev', data: rows.map((r: any) => r.pppoe), itemStyle: { color: '#8B5CF6', borderRadius: [4, 4, 0, 0] } },
    ],
  }
})

const attentionItems = computed(() => {
  const k = data.value?.kpis || {}
  return [
    { label: 'Critical NOC alerts unacknowledged', value: k.criticalAlerts ?? 0, to: '/monitoring', tone: 'bg-rose-500/12 text-rose-600 dark:text-rose-300' },
    { label: 'Open customer & agent support tickets', value: k.openTickets ?? 0, to: '/support', tone: 'bg-amber-500/12 text-amber-600 dark:text-amber-300' },
    { label: 'Outbox RADIUS / SMS retry events', value: k.outboxBacklog ?? 0, to: '/monitoring', tone: 'bg-purple-500/12 text-purple-600 dark:text-purple-300' },
    { label: 'Redeemable + stocked vouchers', value: k.availableVouchers ?? 0, to: '/vouchers', tone: 'bg-sky-500/12 text-sky-600 dark:text-sky-300' },
  ]
})
</script>

<template>
  <div v-if="data" class="space-y-6">
    <PageHeader
      title="MyNet Network & Commercial Overview"
      subtitle="Live FreeRADIUS session load, MikroTik sector backhaul, voucher velocity, and Hubtel/Paystack collections."
      icon="wifi"
    >
      <template #actions>
        <BaseButton variant="secondary" size="sm" icon="arrow-path" :loading="loading" @click="load">
          Refresh telemetry
        </BaseButton>
        <BaseButton to="/zones" size="sm" icon="plus">
          Deploy zone
        </BaseButton>
      </template>
    </PageHeader>

    <!-- Primary KPI Strip -->
    <div class="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
      <StatCard
        label="Concurrent Sessions"
        :value="data.kpis.concurrentSessions"
        :sub="`${data.kpis.activeSubscribers} active subscribers across ${data.kpis.liveZones} live zones`"
        trend="+14.2% peak"
        :trend-up="true"
        icon="signal"
        tone="primary"
      />
      <StatCard
        label="Aggregate Throughput"
        :value="`${data.kpis.totalDownMbps} Mbps`"
        :sub="`Of ${data.kpis.totalCapacityMbps} Mbps fibre capacity`"
        icon="bolt"
        tone="blue"
      />
      <StatCard
        label="30-Day Gross Revenue"
        :value="formatCurrency(data.kpis.monthlyRevenue)"
        :sub="`${data.kpis.activeAgents} active field agents · VAT inclusive`"
        trend="+18.6% MoM"
        :trend-up="true"
        icon="banknotes"
        tone="green"
      />
      <StatCard
        label="RouterOS Fleet Health"
        :value="`${data.kpis.onlineDevices} / ${data.kpis.totalDevices}`"
        :sub="`${data.kpis.openAlerts} active alert(s) · 1 sector PoE fault`"
        icon="server-stack"
        :tone="data.kpis.criticalAlerts > 0 ? 'amber' : 'green'"
      />
    </div>

    <!-- Charts Row -->
    <div class="grid gap-6 lg:grid-cols-5">
      <BaseCard
        class="lg:col-span-3"
        title="Sector Backhaul Utilization (Mbps)"
        subtitle="Real-time RouterOS interface throughput over WireGuard management hub"
        icon="signal"
      >
        <SharedEChart :option="bandwidthChartOption" height="250px" />
      </BaseCard>

      <BaseCard
        class="lg:col-span-2"
        title="7-Day Channel Revenue Mix (GH₵)"
        subtitle="Captive Portal MoMo vs Reseller Vouchers vs Home PPPoE"
        icon="chart-bar-square"
      >
        <SharedEChart :option="revenueChartOption" height="250px" />
      </BaseCard>
    </div>

    <!-- Operational Attention Queue + Zone Live Matrix -->
    <div class="grid gap-6 lg:grid-cols-3">
      <BaseCard
        title="NOC Attention Queue"
        subtitle="Actionable exceptions across AAA, hardware & support"
        icon="bell"
      >
        <div class="space-y-2.5">
          <NuxtLink
            v-for="item in attentionItems"
            :key="item.label"
            :to="item.to"
            class="flex items-center justify-between rounded-xl border border-line bg-canvas/50 px-3.5 py-2.5 transition hover:border-primary/45"
          >
            <span class="text-xs font-medium text-ink-soft">{{ item.label }}</span>
            <span class="rounded-full px-2.5 py-0.5 text-xs font-bold tnum" :class="item.tone">
              {{ item.value }}
            </span>
          </NuxtLink>
        </div>

        <div class="mt-4 space-y-2.5 border-t border-line pt-4">
          <p class="text-[11px] font-bold uppercase tracking-wider text-ink-faint">Unresolved Alerts</p>
          <div
            v-for="alt in data.alerts.slice(0, 3)"
            :key="alt.id"
            class="rounded-xl border border-line p-3 text-xs"
          >
            <div class="flex items-center justify-between gap-2">
              <Badge :tone="statusMeta(alt.severity).tone" size="sm" dot>
                {{ alt.severity }}
              </Badge>
              <span class="text-[11px] text-ink-faint">{{ formatRelativeTime(alt.createdAt) }}</span>
            </div>
            <p class="mt-1.5 font-medium text-ink">{{ alt.message }}</p>
            <div class="mt-2 flex items-center justify-between">
              <span class="text-[11px] text-ink-faint">{{ alt.zoneName || 'Global' }}</span>
              <button
                v-if="!alt.acknowledgedAt"
                type="button"
                class="cursor-pointer text-[11px] font-semibold text-primary hover:underline"
                @click="ackAlert(alt.id)"
              >
                Acknowledge →
              </button>
              <span v-else class="text-[11px] text-emerald-600">Acked by {{ alt.acknowledgedBy }}</span>
            </div>
          </div>
        </div>
      </BaseCard>

      <BaseCard
        class="lg:col-span-2"
        title="Coverage Zones & Sector Load"
        subtitle="One WireGuard tunnel & RADIUS NAS per gateway — 24V passive PoE enforced on mANTBox ax 15s"
        icon="map-pin"
        :pad="false"
      >
        <template #actions>
          <BaseButton to="/zones" variant="ghost" size="sm" icon="arrow-right">Manage zones</BaseButton>
        </template>

        <div class="divide-y divide-line">
          <div
            v-for="z in data.zones"
            :key="z.id"
            class="flex flex-wrap items-center justify-between gap-4 px-5 py-4 transition hover:bg-canvas/40"
          >
            <div class="min-w-[210px] flex-1">
              <div class="flex items-center gap-2.5">
                <span class="font-bold text-ink">{{ z.name }}</span>
                <Badge :tone="statusMeta(z.status).tone" size="sm" dot>{{ statusMeta(z.status).label }}</Badge>
                <code class="rounded bg-canvas px-1.5 py-0.5 text-[11px] text-ink-faint">{{ z.zoneCode }}</code>
              </div>
              <p class="mt-1 text-xs text-ink-faint">
                {{ z.district }} · {{ z.backhaulProvider.replace('_', ' ') }} ({{ z.upstreamDownMbps }}/{{ z.upstreamUpMbps }} Mbps) · WG {{ z.tunnelIp }}
              </p>
            </div>

            <div class="flex items-center gap-6 text-right text-xs tnum">
              <div>
                <p class="text-ink-faint">Devices</p>
                <p class="mt-0.5 font-bold text-ink">{{ z.onlineDevices }}/{{ z.totalDevices }} online</p>
              </div>
              <div>
                <p class="text-ink-faint">Sessions</p>
                <p class="mt-0.5 font-bold text-ink">{{ z.activeSessions }}</p>
              </div>
              <div class="w-28">
                <div class="flex justify-between text-[11px]">
                  <span class="text-ink-faint">Load</span>
                  <span class="font-semibold text-ink">{{ Math.round((z.currentDownMbps / Math.max(1, z.upstreamDownMbps)) * 100) }}%</span>
                </div>
                <div class="mt-1.5 h-2 overflow-hidden rounded-full bg-canvas">
                  <div
                    class="h-full rounded-full bg-primary transition-all"
                    :style="{ width: `${Math.min(100, Math.round((z.currentDownMbps / Math.max(1, z.upstreamDownMbps)) * 100))}%` }"
                  />
                </div>
              </div>
              <div class="min-w-[92px]">
                <p class="text-ink-faint">30d Revenue</p>
                <p class="mt-0.5 font-bold text-ink">{{ formatCurrency(z.monthlyRevenue) }}</p>
              </div>
            </div>
          </div>
        </div>
      </BaseCard>
    </div>

    <!-- Recent Collections Stream -->
    <BaseCard
      title="Recent Portal & Reseller Collections"
      subtitle="Latest Hubtel MoMo, Paystack Card, and Agent Float transactions"
      icon="credit-card"
      :pad="false"
    >
      <template #actions>
        <BaseButton to="/billing" variant="ghost" size="sm" icon="arrow-right">Full ledger</BaseButton>
      </template>
      <DataTable
        :columns="[
          { key: 'internalReference', label: 'Reference' },
          { key: 'subscriberName', label: 'Customer / Payer' },
          { key: 'planName', label: 'Bundle / Purpose' },
          { key: 'zoneName', label: 'Zone' },
          { key: 'paymentMethod', label: 'Channel' },
          { key: 'amount', label: 'Amount', align: 'right' },
          { key: 'status', label: 'Status' },
          { key: 'initiatedAt', label: 'Time' },
        ]"
        :rows="data.recentTransactions"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'internalReference'">
            <code class="font-mono text-xs font-semibold text-ink">{{ row.internalReference }}</code>
          </template>
          <template v-else-if="col.key === 'subscriberName'">
            <div class="font-medium text-ink">{{ row.subscriberName }}</div>
            <div class="text-[11px] text-ink-faint tnum">{{ row.payerMsisdn }}</div>
          </template>
          <template v-else-if="col.key === 'planName'">
            <span class="font-medium text-ink">{{ row.planCode }}</span>
            <span class="ml-1.5 text-xs text-ink-faint">({{ row.purpose }})</span>
          </template>
          <template v-else-if="col.key === 'amount'">
            <span class="font-bold text-ink">{{ formatCurrency(row.amount) }}</span>
          </template>
          <template v-else-if="col.key === 'status'">
            <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
          </template>
          <template v-else-if="col.key === 'initiatedAt'">
            <span class="text-xs text-ink-faint">{{ formatRelativeTime(row.initiatedAt) }}</span>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>
  </div>
</template>
