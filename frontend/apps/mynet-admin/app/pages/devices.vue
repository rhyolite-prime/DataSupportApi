<script setup lang="ts">
import type { MyNetDevice, MyNetZone } from '@mynet-shared/types/models'

useHead({ title: 'RouterOS Fleet' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const devices = ref<MyNetDevice[]>([])
const zones = ref<MyNetZone[]>([])
const zoneFilter = ref('')
const loading = ref(false)
const polling = ref(false)

const showRegister = ref(false)
const diagModal = ref<any>(null)
const pushResult = ref<any>(null)

const form = reactive({
  zoneId: '',
  name: '',
  role: 'AP',
  model: 'mANTBox ax 15s',
  serialNumber: '',
  macAddress: '48:8F:5A:24:08:11',
  tunnelIp: '10.200.1.32',
  poeMode: '24V Passive PoE',
  notes: '',
})

async function load() {
  loading.value = true
  try {
    const [devList, zoneRes] = await Promise.all([
      api.get<MyNetDevice[]>('/admin/devices'),
      api.get<{ zones: MyNetZone[] }>('/admin/zones'),
    ])
    devices.value = devList
    zones.value = zoneRes.zones
    if (!form.zoneId && zoneRes.zones[0]) form.zoneId = zoneRes.zones[0].id
  } catch (e: any) {
    toast.error('Could not load devices', e.message)
  } finally {
    loading.value = false
  }
}

await load()

const filteredDevices = computed(() =>
  zoneFilter.value ? devices.value.filter(d => d.zoneId === zoneFilter.value) : devices.value,
)

async function pollFleet() {
  polling.value = true
  try {
    await api.post('/admin/devices/poll')
    toast.success('Fleet polled', 'RouterOS REST telemetry updated across WireGuard peers.')
    await load()
  } catch (e: any) {
    toast.error('Poll failed', e.message)
  } finally {
    polling.value = false
  }
}

async function pushConfig(dev: MyNetDevice) {
  try {
    const res = await api.post(`/admin/devices/${dev.id}/config`)
    pushResult.value = { deviceName: dev.name, ...res }
    toast.success('Config pushed', `Hotspot + RADIUS + CoA verified on ${dev.name}.`)
    await load()
  } catch (e: any) {
    toast.error('Push failed', e.message)
  }
}

async function openDiagnostics(dev: MyNetDevice) {
  try {
    diagModal.value = await api.get(`/admin/devices/${dev.id}/diagnostics`)
  } catch (e: any) {
    toast.error('Diagnostics failed', e.message)
  }
}

async function rebootDevice(dev: MyNetDevice) {
  try {
    await api.post(`/admin/devices/${dev.id}/reboot`)
    toast.info('Reboot dispatched', `${dev.name} is restarting over WireGuard.`)
    await load()
  } catch (e: any) {
    toast.error('Reboot failed', e.message)
  }
}

async function registerDevice() {
  try {
    await api.post('/admin/devices', { ...form })
    toast.success('Device registered', `${form.name} enrolled & added to radius.nas.`)
    showRegister.value = false
    form.name = ''
    form.serialNumber = ''
    await load()
  } catch (e: any) {
    toast.error('Enrolment failed', e.message)
  }
}
</script>

<template>
  <div class="space-y-6">
    <PageHeader
      title="MikroTik RouterOS Fleet & NAS"
      subtitle="RB5009 / CCR2004 zone gateways and mANTBox ax 15s dual-band Wi-Fi 6 sector radios managed over WireGuard."
      icon="server-stack"
    >
      <template #actions>
        <SelectInput
          v-model="zoneFilter"
          allow-empty
          placeholder="All coverage zones"
          :options="zones.map(z => ({ value: z.id, label: z.name }))"
          class="w-52"
        />
        <BaseButton variant="secondary" size="sm" icon="arrow-path" :loading="polling" @click="pollFleet">
          Poll RouterOS REST
        </BaseButton>
        <BaseButton size="sm" icon="plus" @click="showRegister = true">
          + Enrol device
        </BaseButton>
      </template>
    </PageHeader>

    <!-- Hardware Safety Banner from MagmaNet Implementation Plan -->
    <div class="flex items-start gap-3 rounded-2xl border border-amber-500/30 bg-amber-500/10 px-4 py-3 text-xs text-amber-800 dark:text-amber-200">
      <AppIcon name="exclamation-triangle" :size="18" class="mt-0.5 shrink-0 text-amber-600 dark:text-amber-400" />
      <div>
        <strong>Hardware Voltage Policy:</strong> All <code class="font-mono">mANTBox ax 15s</code> units require
        <strong>18–28 V passive PoE only</strong>. Never connect a standard 48 V 802.3af/at PoE port — use the included 24 V gigabit injector or passive-24V switch profile.
      </div>
    </div>

    <BaseCard :pad="false">
      <DataTable
        :loading="loading"
        :columns="[
          { key: 'name', label: 'Device / Model', sortable: true },
          { key: 'zoneName', label: 'Zone', sortable: true },
          { key: 'role', label: 'Role' },
          { key: 'tunnelIp', label: 'WG / Mgmt IP' },
          { key: 'status', label: 'State' },
          { key: 'cpuLoad', label: 'CPU / RAM', align: 'right', sortable: true },
          { key: 'activeClients', label: 'Clients', align: 'right', sortable: true },
          { key: 'uptimeSeconds', label: 'Uptime', align: 'right' },
          { key: 'actions', label: 'Control', align: 'right' },
        ]"
        :rows="filteredDevices"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'name'">
            <div class="font-semibold text-ink">{{ row.name }}</div>
            <div class="text-[11px] text-ink-faint">{{ row.model }} · ROS {{ row.routerosVersion }} · {{ row.poeMode }}</div>
          </template>
          <template v-else-if="col.key === 'role'">
            <Badge :tone="row.role === 'GATEWAY' ? 'purple' : 'blue'" size="sm">{{ row.role }}</Badge>
          </template>
          <template v-else-if="col.key === 'tunnelIp'">
            <code class="font-mono text-xs font-semibold text-ink">{{ row.tunnelIp }}</code>
            <div class="font-mono text-[10px] text-ink-faint">{{ row.macAddress }}</div>
          </template>
          <template v-else-if="col.key === 'status'">
            <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
          </template>
          <template v-else-if="col.key === 'cpuLoad'">
            <span class="font-semibold text-ink">{{ row.cpuLoad }}%</span>
            <span class="block text-[10px] text-ink-faint">{{ row.freeMemoryMb }} MB free</span>
          </template>
          <template v-else-if="col.key === 'uptimeSeconds'">
            <span class="text-xs font-medium">{{ formatUptime(row.uptimeSeconds) }}</span>
          </template>
          <template v-else-if="col.key === 'actions'">
            <div class="flex items-center justify-end gap-1.5">
              <BaseButton size="sm" variant="secondary" @click="openDiagnostics(row)">Diag</BaseButton>
              <BaseButton size="sm" variant="secondary" @click="pushConfig(row)">Push Config</BaseButton>
              <BaseButton size="sm" variant="ghost" @click="rebootDevice(row)">Reboot</BaseButton>
            </div>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <!-- Diagnostics Modal -->
    <ModalShell
      :open="Boolean(diagModal)"
      :title="`Live Diagnostics · ${diagModal?.device?.name || ''}`"
      subtitle="Queried via RouterOS v7 REST over WireGuard"
      @close="diagModal = null"
    >
      <div v-if="diagModal" class="grid grid-cols-2 gap-3 text-xs">
        <div class="rounded-xl border border-line bg-canvas/50 p-3">
          <span class="text-ink-faint">Tunnel Ping RTT</span>
          <p class="mt-1 text-base font-bold text-ink tnum">{{ diagModal.pingMs ? `${diagModal.pingMs} ms` : 'Unreachable' }}</p>
        </div>
        <div class="rounded-xl border border-line bg-canvas/50 p-3">
          <span class="text-ink-faint">PoE Input Voltage</span>
          <p class="mt-1 text-base font-bold text-emerald-600 tnum">{{ diagModal.voltage }}</p>
        </div>
        <div class="rounded-xl border border-line bg-canvas/50 p-3">
          <span class="text-ink-faint">RADIUS CoA UDP/3799</span>
          <p class="mt-1 font-bold" :class="diagModal.coaPortOpen ? 'text-emerald-600' : 'text-rose-600'">
            {{ diagModal.coaPortOpen ? 'Listening & Verified' : 'No Response' }}
          </p>
        </div>
        <div class="rounded-xl border border-line bg-canvas/50 p-3">
          <span class="text-ink-faint">Board Temperature</span>
          <p class="mt-1 text-base font-bold text-ink tnum">{{ diagModal.temperatureC ? `${diagModal.temperatureC} °C` : '—' }}</p>
        </div>
        <div class="col-span-2 rounded-xl border border-line bg-canvas/50 p-3">
          <span class="text-ink-faint">Radio Channels (20 MHz Non-DFS Plan)</span>
          <p class="mt-1 font-mono text-xs text-ink">2.4 GHz: {{ diagModal.wlan2GhzChannel }} · 5 GHz: {{ diagModal.wlan5GhzChannel }}</p>
        </div>
      </div>
      <template #footer>
        <BaseButton size="sm" @click="diagModal = null">Done</BaseButton>
      </template>
    </ModalShell>

    <!-- Push Config Result Modal -->
    <ModalShell
      :open="Boolean(pushResult)"
      :title="`Provisioning Steps · ${pushResult?.deviceName || ''}`"
      subtitle="Idempotent RouterOS v7 REST provisioning summary"
      @close="pushResult = null"
    >
      <div v-if="pushResult" class="space-y-2.5">
        <div
          v-for="s in pushResult.steps"
          :key="s.step"
          class="flex items-center justify-between rounded-xl border border-line bg-canvas/50 px-4 py-3 text-xs"
        >
          <div>
            <p class="font-mono font-bold text-ink">{{ s.step }}</p>
            <p class="mt-0.5 text-ink-faint">{{ s.detail }}</p>
          </div>
          <Badge tone="green" size="sm" dot>{{ s.status.toUpperCase() }}</Badge>
        </div>
      </div>
      <template #footer>
        <BaseButton size="sm" @click="pushResult = null">Close</BaseButton>
      </template>
    </ModalShell>

    <!-- Enrol Device Modal -->
    <ModalShell :open="showRegister" title="Enrol MikroTik RouterOS Device" subtitle="Registers device & writes shared secret to radius.nas" width="max-w-xl" @close="showRegister = false">
      <div class="grid gap-4 sm:grid-cols-2">
        <FormField label="Zone" required>
          <SelectInput v-model="form.zoneId" :options="zones.map(z => ({ value: z.id, label: z.name }))" />
        </FormField>
        <FormField label="Device Role">
          <SelectInput
            v-model="form.role"
            :options="[
              { value: 'GATEWAY', label: 'Zone Gateway / NAS' },
              { value: 'AP', label: 'Sector Access Point' },
              { value: 'CPE', label: 'Subscriber CPE' },
            ]"
          />
        </FormField>
        <FormField label="Device Hostname" required>
          <TextInput v-model="form.name" placeholder="Teshie Sector East AP" />
        </FormField>
        <FormField label="Hardware Model">
          <SelectInput
            v-model="form.model"
            :options="[
              { value: 'mANTBox ax 15s', label: 'mANTBox ax 15s (Wi-Fi 6 Sector)' },
              { value: 'RB5009UG+S+IN', label: 'RB5009UG+S+IN (Zone Gateway)' },
              { value: 'CCR2004-1G-12S+2XS', label: 'CCR2004 Aggregation Router' },
              { value: 'LHG 5 ac', label: 'LHG 5 CPE' },
            ]"
          />
        </FormField>
        <FormField label="WireGuard Tunnel IP" required>
          <TextInput v-model="form.tunnelIp" placeholder="10.200.1.32" />
        </FormField>
        <FormField label="MAC Address" required>
          <TextInput v-model="form.macAddress" placeholder="48:8F:5A:24:08:11" />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showRegister = false">Cancel</BaseButton>
        <BaseButton size="sm" @click="registerDevice">Enrol Device</BaseButton>
      </template>
    </ModalShell>
  </div>
</template>
