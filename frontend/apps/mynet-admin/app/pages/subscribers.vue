<script setup lang="ts">
import type { MyNetClientDevice, MyNetDataPlan, MyNetSession, MyNetSubscriber, MyNetZone } from '@mynet-shared/types/models'

useHead({ title: 'Subscribers & Live Sessions' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const tab = ref('subscribers')
const search = ref('')
const statusFilter = ref('')

const subscribers = ref<MyNetSubscriber[]>([])
const sessions = ref<MyNetSession[]>([])
const clientDevices = ref<MyNetClientDevice[]>([])
const zones = ref<MyNetZone[]>([])
const plans = ref<MyNetDataPlan[]>([])

const selectedSub = ref<MyNetSubscriber | null>(null)
const showCreate = ref(false)

const newSub = reactive({
  fullName: '',
  msisdn: '',
  email: '',
  subscriberType: 'RESIDENTIAL',
  authType: 'PPPOE',
  homeZoneId: '',
  serviceAddress: '',
  ghanaCardNumber: '',
  planCode: 'MONTH-1',
})

async function load() {
  const [subRes, zoneRes, planList] = await Promise.all([
    api.get<{ subscribers: MyNetSubscriber[]; sessions: MyNetSession[]; clientDevices: MyNetClientDevice[] }>('/admin/subscribers'),
    api.get<{ zones: MyNetZone[] }>('/admin/zones'),
    api.get<MyNetDataPlan[]>('/admin/plans'),
  ])
  subscribers.value = subRes.subscribers
  sessions.value = subRes.sessions
  clientDevices.value = subRes.clientDevices
  zones.value = zoneRes.zones
  plans.value = planList
  if (!newSub.homeZoneId && zoneRes.zones[0]) newSub.homeZoneId = zoneRes.zones[0].id
  if (selectedSub.value) {
    selectedSub.value = subRes.subscribers.find(s => s.id === selectedSub.value?.id) || null
  }
}

await load()

const filteredSubscribers = computed(() => {
  const q = search.value.trim().toLowerCase()
  return subscribers.value.filter((s) => {
    if (statusFilter.value && s.status !== statusFilter.value) return false
    if (!q) return true
    return `${s.fullName} ${s.msisdn} ${s.pppoeUsername || ''} ${s.homeZoneName}`.toLowerCase().includes(q)
  })
})

const selectedSubDevices = computed(() =>
  selectedSub.value ? clientDevices.value.filter(d => d.subscriberId === selectedSub.value!.id) : [],
)

async function runSubscriberAction(sub: MyNetSubscriber, action: 'suspend' | 'resume' | 'extend' | 'disconnect') {
  try {
    await api.post(`/admin/subscribers/${sub.id}/${action}`, action === 'extend' ? { days: 3 } : {})
    toast.success('Subscriber updated', `${action.toUpperCase()} executed for ${sub.fullName}.`)
    await load()
  } catch (e: any) {
    toast.error('Action failed', e.message)
  }
}

async function toggleMacBlock(cdev: MyNetClientDevice) {
  try {
    await api.post(`/admin/client-devices/${cdev.id}/toggle-block`)
    toast.success(cdev.isBlocked ? 'MAC unblocked' : 'MAC blocked')
    await load()
  } catch (e: any) {
    toast.error('MAC update failed', e.message)
  }
}

async function createSubscriber() {
  try {
    await api.post('/admin/subscribers', { ...newSub })
    toast.success('Subscriber onboarded', `${newSub.fullName} added & provisioned to FreeRADIUS.`)
    showCreate.value = false
    Object.assign(newSub, { fullName: '', msisdn: '', email: '', serviceAddress: '', ghanaCardNumber: '' })
    await load()
  } catch (e: any) {
    toast.error('Could not onboard subscriber', e.message)
  }
}
</script>

<template>
  <div class="space-y-6">
    <PageHeader
      title="Subscribers, PPPoE Accounts & Live RADIUS Sessions"
      subtitle="MSISDN & Ghana Card directory, residential PPPoE credentials, live radacct sessions, and CoA disconnect controls."
      icon="user-group"
    >
      <template #actions>
        <BaseButton size="sm" icon="plus" @click="showCreate = true">
          + Onboard PPPoE / Subscriber
        </BaseButton>
      </template>
    </PageHeader>

    <Tabs
      v-model="tab"
      :tabs="[
        { key: 'subscribers', label: 'Subscriber Directory', icon: 'users', count: subscribers.length },
        { key: 'sessions', label: 'Live RADIUS Sessions (radacct)', icon: 'signal', count: sessions.length },
        { key: 'macs', label: 'Client MAC Bindings', icon: 'cpu-chip', count: clientDevices.length },
      ]"
    />

    <template v-if="tab === 'subscribers'">
      <div class="flex flex-wrap items-center gap-3">
        <div class="min-w-[260px] flex-1">
          <TextInput v-model="search" placeholder="Search by phone (MSISDN), name, PPPoE username, or zone…" />
        </div>
        <SelectInput
          v-model="statusFilter"
          allow-empty
          placeholder="All account statuses"
          :options="[
            { value: 'ACTIVE', label: 'Active' },
            { value: 'SUSPENDED', label: 'Suspended' },
            { value: 'CHURNED', label: 'Churned' },
          ]"
          class="w-48"
        />
      </div>

      <BaseCard :pad="false">
        <DataTable
          clickable
          :columns="[
            { key: 'fullName', label: 'Subscriber', sortable: true },
            { key: 'authType', label: 'Access / KYC' },
            { key: 'homeZoneName', label: 'Home Zone', sortable: true },
            { key: 'activePlanCode', label: 'Active Plan' },
            { key: 'usage', label: 'Data Usage' },
            { key: 'status', label: 'Status' },
            { key: 'actions', label: 'Actions', align: 'right' },
          ]"
          :rows="filteredSubscribers"
          @row-click="(r) => selectedSub = r"
        >
          <template #cell="{ col, row }">
            <template v-if="col.key === 'fullName'">
              <div class="flex items-center gap-2">
                <span class="h-2 w-2 rounded-full" :class="row.onlineNow ? 'bg-emerald-500' : 'bg-slate-300'" />
                <div>
                  <p class="font-semibold text-ink">{{ row.fullName }}</p>
                  <p class="text-[11px] text-ink-faint tnum">{{ row.msisdn }} · {{ row.serviceAddress }}</p>
                </div>
              </div>
            </template>
            <template v-else-if="col.key === 'authType'">
              <Badge :tone="row.authType === 'PPPOE' ? 'purple' : 'blue'" size="sm">{{ row.authType }}</Badge>
              <span v-if="row.pppoeUsername" class="ml-1.5 font-mono text-[11px] text-ink-faint">{{ row.pppoeUsername }}</span>
            </template>
            <template v-else-if="col.key === 'activePlanCode'">
              <span class="font-semibold text-ink">{{ row.activePlanCode || '—' }}</span>
              <span v-if="row.expiresAt" class="block text-[10px] text-ink-faint">Exp {{ formatDate(row.expiresAt) }}</span>
            </template>
            <template v-else-if="col.key === 'usage'">
              <div class="w-32 text-xs tnum">
                <span>{{ formatDataVolume(row.usedMb) }} / {{ formatDataVolume(row.purchasedMb) }}</span>
                <div class="mt-1 h-1.5 overflow-hidden rounded-full bg-canvas">
                  <div
                    class="h-full rounded-full bg-primary"
                    :style="{ width: `${Math.min(100, Math.round((row.usedMb / Math.max(1, row.purchasedMb)) * 100))}%` }"
                  />
                </div>
              </div>
            </template>
            <template v-else-if="col.key === 'status'">
              <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
            </template>
            <template v-else-if="col.key === 'actions'">
              <div class="flex items-center justify-end gap-1.5" @click.stop>
                <BaseButton
                  v-if="row.status === 'ACTIVE'"
                  size="sm"
                  variant="secondary"
                  @click="runSubscriberAction(row, 'suspend')"
                >
                  Suspend
                </BaseButton>
                <BaseButton
                  v-else
                  size="sm"
                  variant="success"
                  @click="runSubscriberAction(row, 'resume')"
                >
                  Resume
                </BaseButton>
                <BaseButton size="sm" variant="ghost" @click="runSubscriberAction(row, 'extend')">
                  +3d
                </BaseButton>
              </div>
            </template>
            <template v-else>{{ row[col.key] }}</template>
          </template>
        </DataTable>
      </BaseCard>
    </template>

    <!-- Live RADIUS Sessions -->
    <BaseCard v-else-if="tab === 'sessions'" title="Active FreeRADIUS Sessions" subtitle="Interim accounting updates every 60s over WireGuard" :pad="false">
      <DataTable
        :columns="[
          { key: 'subscriberName', label: 'Subscriber' },
          { key: 'zoneName', label: 'Zone / NAS' },
          { key: 'framedIp', label: 'Framed IP / MAC' },
          { key: 'traffic', label: 'Down / Up', align: 'right' },
          { key: 'startedAt', label: 'Started' },
          { key: 'actions', label: 'CoA', align: 'right' },
        ]"
        :rows="sessions"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'subscriberName'">
            <div class="font-semibold text-ink">{{ row.subscriberName }}</div>
            <div class="text-[11px] text-ink-faint tnum">{{ row.msisdn }}</div>
          </template>
          <template v-else-if="col.key === 'zoneName'">
            <div class="font-medium text-ink">{{ row.zoneName }}</div>
            <div class="font-mono text-[11px] text-ink-faint">{{ row.deviceName }} ({{ row.nasIp }})</div>
          </template>
          <template v-else-if="col.key === 'framedIp'">
            <code class="font-mono text-xs font-semibold text-ink">{{ row.framedIp }}</code>
            <div class="font-mono text-[10px] text-ink-faint">{{ row.macAddress }}</div>
          </template>
          <template v-else-if="col.key === 'traffic'">
            <span class="font-semibold text-ink">{{ formatOctets(row.outputOctets) }}</span>
            <span class="text-xs text-ink-faint"> / {{ formatOctets(row.inputOctets) }}</span>
          </template>
          <template v-else-if="col.key === 'startedAt'">
            <span class="text-xs">{{ formatRelativeTime(row.startedAt) }}</span>
          </template>
          <template v-else-if="col.key === 'actions'">
            <BaseButton
              size="sm"
              variant="danger"
              @click="runSubscriberAction({ id: row.subscriberId, fullName: row.subscriberName } as any, 'disconnect')"
            >
              CoA Kick
            </BaseButton>
          </template>
        </template>
      </DataTable>
    </BaseCard>

    <!-- Client MACs -->
    <BaseCard v-else title="Subscriber Client MAC Addresses (magma_client_devices)" :pad="false">
      <DataTable
        :columns="[
          { key: 'macAddress', label: 'MAC Address' },
          { key: 'label', label: 'Device Label' },
          { key: 'lastSeenAt', label: 'Last Seen' },
          { key: 'isBlocked', label: 'Access Policy' },
          { key: 'actions', label: 'Action', align: 'right' },
        ]"
        :rows="clientDevices"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'macAddress'">
            <code class="font-mono text-xs font-bold text-ink">{{ row.macAddress }}</code>
          </template>
          <template v-else-if="col.key === 'lastSeenAt'">{{ formatRelativeTime(row.lastSeenAt) }}</template>
          <template v-else-if="col.key === 'isBlocked'">
            <Badge :tone="row.isBlocked ? 'red' : 'green'" size="sm" dot>
              {{ row.isBlocked ? `Blocked (${row.blockedReason || 'Policy'})` : 'Permitted' }}
            </Badge>
          </template>
          <template v-else-if="col.key === 'actions'">
            <BaseButton size="sm" :variant="row.isBlocked ? 'secondary' : 'danger'" @click="toggleMacBlock(row)">
              {{ row.isBlocked ? 'Unblock MAC' : 'Block MAC' }}
            </BaseButton>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <!-- Subscriber Detail Drawer/Modal -->
    <ModalShell
      :open="Boolean(selectedSub)"
      :title="selectedSub?.fullName || 'Subscriber Profile'"
      :subtitle="`${selectedSub?.msisdn || ''} · ${selectedSub?.homeZoneName || ''}`"
      width="max-w-xl"
      @close="selectedSub = null"
    >
      <div v-if="selectedSub" class="space-y-4 text-xs">
        <div class="grid grid-cols-3 gap-3 rounded-xl border border-line bg-canvas/50 p-3.5 tnum">
          <div>
            <span class="text-ink-faint">Lifetime Spend</span>
            <p class="mt-0.5 text-base font-bold text-ink">{{ formatCurrency(selectedSub.lifetimeSpend) }}</p>
          </div>
          <div>
            <span class="text-ink-faint">KYC / Ghana Card</span>
            <p class="mt-0.5 font-semibold text-ink">{{ selectedSub.ghanaCardMasked || selectedSub.kycStatus }}</p>
          </div>
          <div>
            <span class="text-ink-faint">Active Bundle</span>
            <p class="mt-0.5 font-semibold text-ink">{{ selectedSub.activePlanName || 'None' }}</p>
          </div>
        </div>

        <div>
          <p class="mb-2 font-bold text-ink">Bound Client Devices ({{ selectedSubDevices.length }})</p>
          <div class="space-y-2">
            <div
              v-for="d in selectedSubDevices"
              :key="d.id"
              class="flex items-center justify-between rounded-xl border border-line px-3.5 py-2.5"
            >
              <div>
                <code class="font-mono font-bold text-ink">{{ d.macAddress }}</code>
                <span class="ml-2 text-ink-faint">{{ d.label }}</span>
              </div>
              <Badge :tone="d.isBlocked ? 'red' : 'green'" size="sm">{{ d.isBlocked ? 'Blocked' : 'Allowed' }}</Badge>
            </div>
          </div>
        </div>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="selectedSub = null">Close</BaseButton>
      </template>
    </ModalShell>

    <!-- Onboard Subscriber Modal -->
    <ModalShell :open="showCreate" title="Onboard Fixed-Wireless PPPoE or Hotspot Subscriber" width="max-w-xl" @close="showCreate = false">
      <div class="grid gap-4 sm:grid-cols-2">
        <FormField label="Full Name / Business Name" required>
          <TextInput v-model="newSub.fullName" placeholder="Mensah Enterprise" />
        </FormField>
        <FormField label="Phone Number (MSISDN)" required>
          <TextInput v-model="newSub.msisdn" placeholder="0244900112" />
        </FormField>
        <FormField label="Subscriber Segment">
          <SelectInput
            v-model="newSub.subscriberType"
            :options="[
              { value: 'RESIDENTIAL', label: 'Residential Home CPE' },
              { value: 'BUSINESS', label: 'Small Business / Shop' },
              { value: 'WALK_IN', label: 'Walk-in Hotspot' },
            ]"
          />
        </FormField>
        <FormField label="Authentication Mode">
          <SelectInput
            v-model="newSub.authType"
            :options="[
              { value: 'PPPOE', label: 'PPPoE Always-On CPE' },
              { value: 'HOTSPOT', label: 'Captive Portal Hotspot' },
            ]"
          />
        </FormField>
        <FormField label="Home Zone">
          <SelectInput v-model="newSub.homeZoneId" :options="zones.map(z => ({ value: z.id, label: z.name }))" />
        </FormField>
        <FormField label="Initial Internet Plan">
          <SelectInput v-model="newSub.planCode" :options="plans.map(p => ({ value: p.planCode, label: `${p.name} (${formatCurrency(p.price)})` }))" />
        </FormField>
        <FormField label="Ghana Card Number (Encrypted at rest)">
          <TextInput v-model="newSub.ghanaCardNumber" placeholder="GHA-721894012-4" />
        </FormField>
        <FormField label="Installation / Stall Address">
          <TextInput v-model="newSub.serviceAddress" placeholder="Madina Zongo Junction, Hse 9" />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showCreate = false">Cancel</BaseButton>
        <BaseButton size="sm" @click="createSubscriber">Provision Subscriber</BaseButton>
      </template>
    </ModalShell>
  </div>
</template>
