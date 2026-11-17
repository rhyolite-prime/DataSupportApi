<script setup lang="ts">
import type { MyNetZone, MyNetZoneCost } from '@mynet-shared/types/models'

useHead({ title: 'Zones & Coverage Sites' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const tab = ref('zones')
const zones = ref<MyNetZone[]>([])
const costs = ref<MyNetZoneCost[]>([])
const loading = ref(false)

const showCreate = ref(false)
const showCostModal = ref(false)
const saving = ref(false)

const newZone = reactive({
  zoneCode: '',
  name: '',
  district: '',
  region: 'Greater Accra',
  backhaulProvider: 'MTN_FIBRE',
  upstreamDownMbps: 100,
  upstreamUpMbps: 50,
  upstreamMonthlyCost: 1200,
  tunnelIp: '10.200.1.50',
  status: 'BUILD',
  notes: '',
})

const newCost = reactive({
  zoneId: '',
  category: 'BACKHAUL',
  vendor: '',
  reference: '',
  amount: 500,
  notes: '',
})

async function load() {
  loading.value = true
  try {
    const res = await api.get<{ zones: MyNetZone[]; costs: MyNetZoneCost[] }>('/admin/zones')
    zones.value = res.zones
    costs.value = res.costs
    if (!newCost.zoneId && res.zones[0]) newCost.zoneId = res.zones[0].id
  } catch (e: any) {
    toast.error('Failed to load zones', e.message)
  } finally {
    loading.value = false
  }
}

await load()

const zonePnlRows = computed(() =>
  zones.value.map((z) => {
    const opCosts = costs.value.filter(c => c.zoneId === z.id).reduce((s, c) => s + c.amount, 0)
    const margin = z.monthlyRevenue - opCosts
    return {
      ...z,
      operatingCost: opCosts,
      ebitda: margin,
      marginPct: z.monthlyRevenue > 0 ? Math.round((margin / z.monthlyRevenue) * 100) : 0,
    }
  }),
)

async function createZone() {
  saving.value = true
  try {
    await api.post('/admin/zones', { ...newZone })
    toast.success('Zone created', `${newZone.name} registered in MyNet control plane.`)
    showCreate.value = false
    Object.assign(newZone, { zoneCode: '', name: '', district: '', notes: '' })
    await load()
  } catch (e: any) {
    toast.error('Could not create zone', e.message)
  } finally {
    saving.value = false
  }
}

async function setZoneStatus(zone: MyNetZone, status: MyNetZone['status']) {
  try {
    await api.put(`/admin/zones/${zone.id}`, { status })
    toast.success('Zone status updated', `${zone.name} is now ${status}.`)
    await load()
  } catch (e: any) {
    toast.error('Update failed', e.message)
  }
}

async function recordCost() {
  if (!newCost.zoneId) return
  saving.value = true
  try {
    await api.post(`/admin/zones/${newCost.zoneId}/costs`, { ...newCost })
    toast.success('Cost recorded', 'Zone operating expense added to monthly P&L.')
    showCostModal.value = false
    Object.assign(newCost, { vendor: '', reference: '', amount: 500, notes: '' })
    await load()
  } catch (e: any) {
    toast.error('Failed to record cost', e.message)
  } finally {
    saving.value = false
  }
}
</script>

<template>
  <div class="space-y-6">
    <PageHeader
      title="Zones & Coverage Sites"
      subtitle="Manage community WiFi & PPPoE coverage sectors, CGNAT WireGuard tunnels, fibre backhaul, and site P&L."
      icon="map-pin"
    >
      <template #actions>
        <BaseButton variant="secondary" size="sm" icon="banknotes" @click="showCostModal = true">
          + Record site cost
        </BaseButton>
        <BaseButton size="sm" icon="plus" @click="showCreate = true">
          + New coverage zone
        </BaseButton>
      </template>
    </PageHeader>

    <Tabs
      v-model="tab"
      :tabs="[
        { key: 'zones', label: 'Coverage Zones', icon: 'map-pin', count: zones.length },
        { key: 'pnl', label: 'Zone Profitability (P&L)', icon: 'scale' },
        { key: 'costs', label: 'Operating Cost Ledger', icon: 'banknotes', count: costs.length },
      ]"
    />

    <!-- Zones Grid -->
    <div v-if="tab === 'zones'" class="grid gap-5 lg:grid-cols-2">
      <BaseCard v-for="z in zones" :key="z.id">
        <div class="flex flex-wrap items-start justify-between gap-3">
          <div>
            <div class="flex items-center gap-2.5">
              <h3 class="text-base font-bold text-ink">{{ z.name }}</h3>
              <Badge :tone="statusMeta(z.status).tone" dot>{{ statusMeta(z.status).label }}</Badge>
            </div>
            <p class="mt-1 text-xs text-ink-faint">
              Code <code class="rounded bg-canvas px-1.5 py-0.5 font-mono text-[11px] text-ink">{{ z.zoneCode }}</code> · {{ z.district }}, {{ z.region }}
            </p>
          </div>

          <div class="flex items-center gap-1.5">
            <BaseButton
              v-if="z.status !== 'LIVE'"
              size="sm"
              variant="success"
              icon="play-circle"
              @click="setZoneStatus(z, 'LIVE')"
            >
              Open Live
            </BaseButton>
            <BaseButton
              v-if="z.status === 'LIVE'"
              size="sm"
              variant="secondary"
              icon="pause-circle"
              @click="setZoneStatus(z, 'MAINTENANCE')"
            >
              Maintenance
            </BaseButton>
          </div>
        </div>

        <div class="mt-4 grid grid-cols-2 gap-3 rounded-xl border border-line bg-canvas/50 p-3.5 text-xs sm:grid-cols-4 tnum">
          <div>
            <span class="text-ink-faint">Backhaul</span>
            <p class="mt-0.5 font-bold text-ink">{{ z.upstreamDownMbps }}/{{ z.upstreamUpMbps }} Mbps</p>
            <span class="text-[10px] text-ink-faint">{{ z.backhaulProvider.replace('_', ' ') }}</span>
          </div>
          <div>
            <span class="text-ink-faint">WireGuard NAS</span>
            <p class="mt-0.5 font-mono font-bold text-ink">{{ z.tunnelIp }}</p>
            <span class="text-[10px] text-ink-faint">{{ z.upstreamIsCgnat ? 'CGNAT Traversal' : 'Static IP' }}</span>
          </div>
          <div>
            <span class="text-ink-faint">Live Sessions</span>
            <p class="mt-0.5 font-bold text-ink">{{ z.activeSessions }}</p>
            <span class="text-[10px] text-ink-faint">{{ z.onlineDevices }}/{{ z.totalDevices }} devices up</span>
          </div>
          <div>
            <span class="text-ink-faint">30d Revenue</span>
            <p class="mt-0.5 font-bold text-ink">{{ formatCurrency(z.monthlyRevenue) }}</p>
            <span class="text-[10px] text-ink-faint">Cost {{ formatCurrency(z.upstreamMonthlyCost) }}/mo</span>
          </div>
        </div>

        <p v-if="z.notes" class="mt-3 text-xs leading-relaxed text-ink-soft">{{ z.notes }}</p>
      </BaseCard>
    </div>

    <!-- Zone P&L -->
    <BaseCard v-else-if="tab === 'pnl'" title="Monthly Site Profitability (Revenue vs Backhaul/Power/Rent)" icon="scale" :pad="false">
      <DataTable
        :columns="[
          { key: 'name', label: 'Zone', sortable: true },
          { key: 'status', label: 'Status' },
          { key: 'activeSubscribers', label: 'Subscribers', align: 'right', sortable: true },
          { key: 'monthlyRevenue', label: 'Gross Revenue', align: 'right', sortable: true },
          { key: 'operatingCost', label: 'Direct OpEx', align: 'right', sortable: true },
          { key: 'ebitda', label: 'Net Site Contribution', align: 'right', sortable: true },
          { key: 'marginPct', label: 'Margin %', align: 'right', sortable: true },
        ]"
        :rows="zonePnlRows"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'name'">
            <span class="font-semibold text-ink">{{ row.name }}</span>
            <span class="ml-1.5 text-xs text-ink-faint">({{ row.zoneCode }})</span>
          </template>
          <template v-else-if="col.key === 'status'">
            <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
          </template>
          <template v-else-if="col.key === 'monthlyRevenue'">{{ formatCurrency(row.monthlyRevenue) }}</template>
          <template v-else-if="col.key === 'operatingCost'">{{ formatCurrency(row.operatingCost) }}</template>
          <template v-else-if="col.key === 'ebitda'">
            <span class="font-bold" :class="row.ebitda >= 0 ? 'text-emerald-600 dark:text-emerald-400' : 'text-rose-600'">
              {{ formatCurrency(row.ebitda) }}
            </span>
          </template>
          <template v-else-if="col.key === 'marginPct'">
            <Badge :tone="row.marginPct >= 50 ? 'green' : row.marginPct > 0 ? 'amber' : 'gray'" size="sm">
              {{ row.marginPct }}%
            </Badge>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <!-- Operating Costs -->
    <BaseCard v-else title="Site Operating Costs (magma_zone_costs)" icon="banknotes" :pad="false">
      <DataTable
        :columns="[
          { key: 'zoneName', label: 'Zone' },
          { key: 'category', label: 'Category' },
          { key: 'vendor', label: 'Vendor' },
          { key: 'reference', label: 'Reference' },
          { key: 'periodStart', label: 'Period' },
          { key: 'amount', label: 'Amount', align: 'right', sortable: true },
        ]"
        :rows="costs"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'category'">
            <Badge tone="blue" size="sm">{{ row.category }}</Badge>
          </template>
          <template v-else-if="col.key === 'periodStart'">
            <span class="text-xs text-ink-faint">{{ row.periodStart }} → {{ row.periodEnd }}</span>
          </template>
          <template v-else-if="col.key === 'amount'">
            <span class="font-bold text-ink">{{ formatCurrency(row.amount) }}</span>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <!-- Create Zone Modal -->
    <ModalShell :open="showCreate" title="Register New MyNet Coverage Zone" subtitle="Creates zone & assigns WireGuard management IP" width="max-w-xl" @close="showCreate = false">
      <form class="grid gap-4 sm:grid-cols-2" @submit.prevent="createZone">
        <FormField label="Zone Code" hint="Captive portal identifier (e.g. DANSOMAN)" required>
          <TextInput v-model="newZone.zoneCode" placeholder="DANSOMAN" required />
        </FormField>
        <FormField label="Commercial Name" required>
          <TextInput v-model="newZone.name" placeholder="Dansoman Roundabout" required />
        </FormField>
        <FormField label="District / Municipality" required>
          <TextInput v-model="newZone.district" placeholder="Ablekuma West" required />
        </FormField>
        <FormField label="Backhaul Provider">
          <SelectInput
            v-model="newZone.backhaulProvider"
            :options="[
              { value: 'MTN_FIBRE', label: 'MTN Business Fibre' },
              { value: 'TELECEL_FIBRE', label: 'Telecel Business Fibre' },
              { value: 'STARLINK_BIZ', label: 'Starlink Priority Backhaul' },
            ]"
          />
        </FormField>
        <FormField label="Downstream (Mbps)">
          <TextInput v-model="newZone.upstreamDownMbps" type="number" min="10" />
        </FormField>
        <FormField label="Upstream (Mbps)">
          <TextInput v-model="newZone.upstreamUpMbps" type="number" min="5" />
        </FormField>
        <FormField label="WireGuard Tunnel IP">
          <TextInput v-model="newZone.tunnelIp" placeholder="10.200.1.50" />
        </FormField>
        <FormField label="Monthly Fibre Cost (GH₵)">
          <TextInput v-model="newZone.upstreamMonthlyCost" type="number" min="0" />
        </FormField>
        <div class="sm:col-span-2">
          <FormField label="Site Engineering Notes">
            <TextareaInput v-model="newZone.notes" placeholder="Mast height, PoE distribution, UPS runtime…" />
          </FormField>
        </div>
      </form>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showCreate = false">Cancel</BaseButton>
        <BaseButton size="sm" :loading="saving" @click="createZone">Create Zone</BaseButton>
      </template>
    </ModalShell>

    <!-- Record Cost Modal -->
    <ModalShell :open="showCostModal" title="Record Zone Operating Expense" subtitle="Feeds monthly zone EBITDA & P&L reporting" @close="showCostModal = false">
      <div class="space-y-4">
        <FormField label="Zone" required>
          <SelectInput v-model="newCost.zoneId" :options="zones.map(z => ({ value: z.id, label: `${z.name} (${z.zoneCode})` }))" />
        </FormField>
        <div class="grid grid-cols-2 gap-3">
          <FormField label="Category">
            <SelectInput
              v-model="newCost.category"
              :options="[
                { value: 'BACKHAUL', label: 'Backhaul Fibre' },
                { value: 'POWER', label: 'ECG / Solar Power' },
                { value: 'RENT', label: 'Mast / Roof Rent' },
                { value: 'MAINTENANCE', label: 'Field Maintenance' },
              ]"
            />
          </FormField>
          <FormField label="Amount (GH₵)" required>
            <TextInput v-model="newCost.amount" type="number" min="1" step="0.01" />
          </FormField>
        </div>
        <FormField label="Vendor / Payee" required>
          <TextInput v-model="newCost.vendor" placeholder="ECG Prepaid / MTN Business" />
        </FormField>
        <FormField label="Invoice / Receipt Reference">
          <TextInput v-model="newCost.reference" placeholder="INV-2026-881" />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showCostModal = false">Cancel</BaseButton>
        <BaseButton size="sm" :loading="saving" @click="recordCost">Save Cost</BaseButton>
      </template>
    </ModalShell>
  </div>
</template>
