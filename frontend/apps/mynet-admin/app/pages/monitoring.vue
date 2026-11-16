<script setup lang="ts">
useHead({ title: 'Monitoring, Alerts & Outbox' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const data = ref<any>(null)
const draining = ref(false)
const openAlertCount = useState<number>('mynet-admin-alert-count', () => 0)

async function load() {
  data.value = await api.get('/admin/monitoring')
  openAlertCount.value = data.value.alerts.filter((a: any) => !a.resolvedAt).length
}

await load()

async function alertAction(id: string, action: 'ack' | 'resolve') {
  try {
    await api.post(`/admin/alerts/${id}/${action}`)
    toast.success(action === 'ack' ? 'Alert acknowledged' : 'Alert resolved')
    await load()
  } catch (e: any) {
    toast.error('Failed to update alert', e.message)
  }
}

async function drainOutbox() {
  draining.value = true
  try {
    const res = await api.post<any>('/admin/outbox/drain')
    toast.success('Outbox drained', `${res.processed} event(s) dispatched to FreeRADIUS & SMS worker.`)
    await load()
  } catch (e: any) {
    toast.error('Outbox drain failed', e.message)
  } finally {
    draining.value = false
  }
}
</script>

<template>
  <div v-if="data" class="space-y-6">
    <PageHeader
      title="NOC Monitoring, Alerts & Async Outbox"
      subtitle="Control-plane /healthz probes, FreeRADIUS auth latency, NOC incident acknowledgements, and transactional outbox worker."
      icon="signal"
    >
      <template #actions>
        <BaseButton size="sm" icon="arrow-path" :loading="draining" @click="drainOutbox">
          Drain Outbox Queue
        </BaseButton>
      </template>
    </PageHeader>

    <!-- Health Strip -->
    <div class="grid gap-3 sm:grid-cols-3 lg:grid-cols-6">
      <div
        v-for="(label, key) in {
          api: 'Control Plane API',
          postgres: 'PostgreSQL 16',
          redis: 'Redis Rate-Limit',
          freeradiusPrimary: 'FreeRADIUS Pi #1',
          freeradiusStandby: 'FreeRADIUS Pi #2',
          wireguardHub: 'WireGuard Hub',
        }"
        :key="key"
        class="rounded-2xl border border-line bg-surface p-4"
      >
        <p class="text-[11px] font-semibold text-ink-faint">{{ label }}</p>
        <div class="mt-2 flex items-center gap-2">
          <span class="h-2.5 w-2.5 rounded-full bg-emerald-500" />
          <span class="text-xs font-bold text-ink">{{ data.health[key] }}</span>
        </div>
      </div>
    </div>

    <div class="grid gap-6 lg:grid-cols-2">
      <!-- Alerts -->
      <BaseCard title="NOC Incident & Threshold Alerts" subtitle="Acknowledge or resolve operational alerts" icon="bell" :pad="false">
        <div class="divide-y divide-line">
          <div v-for="a in data.alerts" :key="a.id" class="p-4">
            <div class="flex items-center justify-between gap-2">
              <div class="flex items-center gap-2">
                <Badge :tone="statusMeta(a.severity).tone" size="sm" dot>{{ a.severity }}</Badge>
                <code class="font-mono text-xs font-bold text-ink">{{ a.alertType }}</code>
              </div>
              <span class="text-xs text-ink-faint">{{ formatRelativeTime(a.createdAt) }}</span>
            </div>
            <p class="mt-2 text-sm text-ink">{{ a.message }}</p>
            <div class="mt-3 flex flex-wrap items-center justify-between gap-2 text-xs">
              <span class="text-ink-faint">
                {{ a.zoneName || 'Global' }} {{ a.deviceName ? `· ${a.deviceName}` : '' }}
              </span>
              <div v-if="!a.resolvedAt" class="flex items-center gap-1.5">
                <BaseButton v-if="!a.acknowledgedAt" size="sm" variant="secondary" @click="alertAction(a.id, 'ack')">
                  Acknowledge
                </BaseButton>
                <BaseButton size="sm" variant="success" @click="alertAction(a.id, 'resolve')">
                  Resolve
                </BaseButton>
              </div>
              <Badge v-else tone="green" size="sm">Resolved</Badge>
            </div>
          </div>
        </div>
      </BaseCard>

      <!-- Transactional Outbox -->
      <BaseCard title="Transactional Outbox (magma_outbox)" subtitle="Asynchronous RADIUS projection, CoA & Hubtel SMS delivery" icon="server-stack" :pad="false">
        <DataTable
          :columns="[
            { key: 'eventType', label: 'Event' },
            { key: 'summary', label: 'Payload Summary' },
            { key: 'attempts', label: 'Tries', align: 'right' },
            { key: 'status', label: 'State' },
          ]"
          :rows="data.outbox"
        >
          <template #cell="{ col, row }">
            <template v-if="col.key === 'eventType'">
              <code class="font-mono text-xs font-bold text-ink">{{ row.eventType }}</code>
            </template>
            <template v-else-if="col.key === 'summary'">
              <div class="max-w-xs truncate text-xs text-ink">{{ row.summary }}</div>
              <div v-if="row.lastError" class="text-[11px] text-rose-600">{{ row.lastError }}</div>
            </template>
            <template v-else-if="col.key === 'status'">
              <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
            </template>
            <template v-else>{{ row[col.key] }}</template>
          </template>
        </DataTable>
      </BaseCard>
    </div>
  </div>
</template>
