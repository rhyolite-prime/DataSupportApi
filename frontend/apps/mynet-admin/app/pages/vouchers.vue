<script setup lang="ts">
import type { MyNetAgent, MyNetDataPlan, MyNetVoucher, MyNetVoucherBatch, MyNetZone } from '@mynet-shared/types/models'

useHead({ title: 'Voucher Batches & Stock' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const tab = ref('batches')
const batches = ref<MyNetVoucherBatch[]>([])
const vouchers = ref<MyNetVoucher[]>([])
const plans = ref<MyNetDataPlan[]>([])
const zones = ref<MyNetZone[]>([])
const agents = ref<MyNetAgent[]>([])

const showIssue = ref(false)
const assignTargetBatch = ref<MyNetVoucherBatch | null>(null)
const assignAgentId = ref('')
const batchFilter = ref('')

const form = reactive({
  planId: '',
  zoneId: '',
  quantity: 20,
  expiresInDays: 90,
  ownerAgentId: '',
})

async function load() {
  const [vRes, pList, zRes, aRes] = await Promise.all([
    api.get<{ batches: MyNetVoucherBatch[]; vouchers: MyNetVoucher[] }>('/admin/vouchers'),
    api.get<MyNetDataPlan[]>('/admin/plans'),
    api.get<{ zones: MyNetZone[] }>('/admin/zones'),
    api.get<{ agents: MyNetAgent[] }>('/admin/agents'),
  ])
  batches.value = vRes.batches
  vouchers.value = vRes.vouchers
  plans.value = pList
  zones.value = zRes.zones
  agents.value = aRes.agents
  if (!form.planId && pList[1]) form.planId = pList[1].id
  if (!form.zoneId && zRes.zones[0]) form.zoneId = zRes.zones[0].id
  if (!assignAgentId.value && aRes.agents[0]) assignAgentId.value = aRes.agents[0].id
}

await load()

const filteredVouchers = computed(() =>
  batchFilter.value ? vouchers.value.filter(v => v.batchId === batchFilter.value) : vouchers.value,
)

async function issueBatch() {
  try {
    await api.post('/admin/vouchers/batches', { ...form })
    toast.success('Batch minted', `${form.quantity} vouchers generated with HMAC-SHA256 + AES-GCM envelope.`)
    showIssue.value = false
    await load()
  } catch (e: any) {
    toast.error('Issue failed', e.message)
  }
}

async function assignBatch() {
  if (!assignTargetBatch.value || !assignAgentId.value) return
  try {
    await api.post(`/admin/vouchers/batches/${assignTargetBatch.value.id}/assign`, { agentId: assignAgentId.value })
    toast.success('Stock assigned', `Batch ${assignTargetBatch.value.batchCode} transferred to agent wallet.`)
    assignTargetBatch.value = null
    await load()
  } catch (e: any) {
    toast.error('Assignment failed', e.message)
  }
}

async function voidBatch(batch: MyNetVoucherBatch) {
  try {
    await api.post(`/admin/vouchers/batches/${batch.id}/void`)
    toast.info('Batch voided', `Unredeemed PINs in ${batch.batchCode} invalidated.`)
    await load()
  } catch (e: any) {
    toast.error('Void failed', e.message)
  }
}

function downloadCsv(batch: MyNetVoucherBatch) {
  if (!import.meta.client) return
  const rows = vouchers.value.filter(v => v.batchId === batch.id)
  const csv = [
    'Serial,VoucherCode,PlanCode,Zone,PriceGHS,Status,ExpiresAt',
    ...rows.map(r => `${r.serial},${r.code},${r.planCode},${r.zoneCode},${r.price.toFixed(2)},${r.status},${r.expiresAt}`),
  ].join('\n')
  const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `${batch.batchCode}-vouchers.csv`
  a.click()
  URL.revokeObjectURL(url)
  toast.success('CSV exported', `Downloaded ${rows.length} voucher rows for ${batch.batchCode}.`)
}
</script>

<template>
  <div class="space-y-6">
    <PageHeader
      title="Voucher Batches & Distribution Inventory"
      subtitle="Mint HMAC-verified voucher batches, assign stock to MyNet field agents, export print sheets, or void compromised stock."
      icon="ticket"
    >
      <template #actions>
        <BaseButton size="sm" icon="plus" @click="showIssue = true">
          + Issue voucher batch
        </BaseButton>
      </template>
    </PageHeader>

    <Tabs
      v-model="tab"
      :tabs="[
        { key: 'batches', label: 'Voucher Batches', icon: 'cube', count: batches.length },
        { key: 'vouchers', label: 'Individual Voucher PINs', icon: 'ticket', count: vouchers.length },
      ]"
    />

    <BaseCard v-if="tab === 'batches'" :pad="false">
      <DataTable
        :columns="[
          { key: 'batchCode', label: 'Batch', sortable: true },
          { key: 'planName', label: 'Internet Plan' },
          { key: 'zoneName', label: 'Zone' },
          { key: 'ownerAgentName', label: 'Custody / Owner' },
          { key: 'stock', label: 'Stock Breakdown' },
          { key: 'unitPrice', label: 'Retail / Wholesale', align: 'right' },
          { key: 'status', label: 'Status' },
          { key: 'actions', label: 'Actions', align: 'right' },
        ]"
        :rows="batches"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'batchCode'">
            <code class="font-mono text-xs font-bold text-ink">{{ row.batchCode }}</code>
            <div class="text-[10px] text-ink-faint">{{ row.quantity }} total PINs</div>
          </template>
          <template v-else-if="col.key === 'ownerAgentName'">
            <span v-if="row.ownerAgentName" class="font-medium text-ink">{{ row.ownerAgentName }}</span>
            <Badge v-else tone="gray" size="sm">MyNet Vault</Badge>
          </template>
          <template v-else-if="col.key === 'stock'">
            <div class="flex items-center gap-1.5 text-[11px] tnum">
              <Badge tone="green" size="sm">{{ row.availableCount }} vault</Badge>
              <Badge tone="blue" size="sm">{{ row.assignedCount }} agent</Badge>
              <Badge tone="purple" size="sm">{{ row.soldCount + row.redeemedCount }} used</Badge>
            </div>
          </template>
          <template v-else-if="col.key === 'unitPrice'">
            <span class="font-bold text-ink">{{ formatCurrency(row.unitPrice) }}</span>
            <span class="block text-[10px] text-ink-faint">WS {{ formatCurrency(row.wholesalePrice) }}</span>
          </template>
          <template v-else-if="col.key === 'status'">
            <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
          </template>
          <template v-else-if="col.key === 'actions'">
            <div class="flex items-center justify-end gap-1.5">
              <BaseButton size="sm" variant="secondary" icon="arrow-down-tray" @click="downloadCsv(row)">
                CSV
              </BaseButton>
              <BaseButton
                v-if="row.availableCount > 0 && row.status === 'ACTIVE'"
                size="sm"
                variant="secondary"
                @click="assignTargetBatch = row"
              >
                Assign Agent
              </BaseButton>
              <BaseButton
                v-if="row.status === 'ACTIVE'"
                size="sm"
                variant="ghost"
                @click="voidBatch(row)"
              >
                Void
              </BaseButton>
            </div>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <div v-else class="space-y-4">
      <div class="flex items-center justify-between gap-3">
        <SelectInput
          v-model="batchFilter"
          allow-empty
          placeholder="All voucher batches"
          :options="batches.map(b => ({ value: b.id, label: `${b.batchCode} — ${b.planCode}` }))"
          class="max-w-xs"
        />
      </div>
      <BaseCard :pad="false">
        <DataTable
          :columns="[
            { key: 'serial', label: 'Serial' },
            { key: 'code', label: 'Decrypted PIN' },
            { key: 'planCode', label: 'Plan' },
            { key: 'zoneName', label: 'Zone' },
            { key: 'agentName', label: 'Assigned Agent' },
            { key: 'price', label: 'Retail', align: 'right' },
            { key: 'status', label: 'Status' },
          ]"
          :rows="filteredVouchers"
        >
          <template #cell="{ col, row }">
            <template v-if="col.key === 'serial'">
              <code class="font-mono text-xs text-ink-faint">{{ row.serial }}</code>
            </template>
            <template v-else-if="col.key === 'code'">
              <code class="rounded bg-canvas px-2 py-0.5 font-mono text-xs font-bold text-ink">{{ row.code }}</code>
            </template>
            <template v-else-if="col.key === 'price'">{{ formatCurrency(row.price) }}</template>
            <template v-else-if="col.key === 'status'">
              <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
            </template>
            <template v-else>{{ row[col.key] || '—' }}</template>
          </template>
        </DataTable>
      </BaseCard>
    </div>

    <!-- Issue Batch Modal -->
    <ModalShell :open="showIssue" title="Mint New MyNet Voucher Batch" subtitle="Generates cryptographic HMAC codes & optional direct agent assignment" @close="showIssue = false">
      <div class="space-y-4">
        <FormField label="Internet Plan" required>
          <SelectInput
            v-model="form.planId"
            :options="plans.map(p => ({ value: p.id, label: `${p.name} (${formatCurrency(p.price)})` }))"
          />
        </FormField>
        <FormField label="Zone Scope" required>
          <SelectInput v-model="form.zoneId" :options="zones.map(z => ({ value: z.id, label: z.name }))" />
        </FormField>
        <div class="grid grid-cols-2 gap-3">
          <FormField label="Quantity (PINs)" required>
            <TextInput v-model="form.quantity" type="number" min="1" max="100" />
          </FormField>
          <FormField label="Expiry Window (Days)">
            <TextInput v-model="form.expiresInDays" type="number" min="7" max="365" />
          </FormField>
        </div>
        <FormField label="Assign Directly to Field Agent (Optional)">
          <SelectInput
            v-model="form.ownerAgentId"
            allow-empty
            placeholder="Keep in MyNet Business Vault"
            :options="agents.map(a => ({ value: a.id, label: `${a.name} (${a.agentCode})` }))"
          />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showIssue = false">Cancel</BaseButton>
        <BaseButton size="sm" @click="issueBatch">Mint Batch</BaseButton>
      </template>
    </ModalShell>

    <!-- Assign Batch Modal -->
    <ModalShell
      :open="Boolean(assignTargetBatch)"
      :title="`Assign Batch ${assignTargetBatch?.batchCode || ''} to Agent`"
      @close="assignTargetBatch = null"
    >
      <FormField label="Select Active Reseller Agent" required>
        <SelectInput
          v-model="assignAgentId"
          :options="agents.map(a => ({ value: a.id, label: `${a.name} (${a.territoryZoneName})` }))"
        />
      </FormField>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="assignTargetBatch = null">Cancel</BaseButton>
        <BaseButton size="sm" @click="assignBatch">Transfer Stock</BaseButton>
      </template>
    </ModalShell>
  </div>
</template>
