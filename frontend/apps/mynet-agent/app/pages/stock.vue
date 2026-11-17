<script setup lang="ts">
import type { MyNetVoucher } from '@mynet-shared/types/models'

useHead({ title: 'Voucher Stock & Batches' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()
const agentFloat = useState<number>('mynet-agent-float-badge', () => 0)

const tab = ref('pins')
const data = ref<any>(null)
const showPurchase = ref(false)
const selectedVoucher = ref<MyNetVoucher | null>(null)
const form = reactive({
  planId: '',
  zoneId: '',
  quantity: 10,
})

async function load() {
  data.value = await api.get('/agent/stock')
  if (data.value?.agent) {
    agentFloat.value = data.value.agent.floatBalance
    if (!form.zoneId) form.zoneId = data.value.agent.territoryZoneId
  }
  if (!form.planId && data.value?.plans?.[1]) {
    form.planId = data.value.plans[1].id
  }
}

await load()

const selectedPlan = computed(() => data.value?.plans?.find((p: any) => p.id === form.planId))
const wholesaleEstimate = computed(() => {
  if (!selectedPlan.value || !data.value?.agent) return 0
  const unit = selectedPlan.value.price * (1 - data.value.agent.commissionRate)
  return Number((unit * Number(form.quantity || 0)).toFixed(2))
})

async function buyStockBatch() {
  try {
    await api.post('/agent/stock/purchase', { ...form })
    toast.success('Wholesale batch purchased', `${form.quantity} discounted vouchers added to your kiosk stock.`)
    showPurchase.value = false
    await load()
  } catch (e: any) {
    toast.error('Purchase failed', e.message)
  }
}
</script>

<template>
  <div v-if="data" class="space-y-6">
    <PageHeader
      title="Voucher Stock & Wholesale Batches"
      subtitle="Inspect pre-assigned voucher PINs, reprint customer QR slips, or buy discounted wholesale batches from your float."
      icon="ticket"
    >
      <template #actions>
        <BaseButton size="sm" icon="plus" @click="showPurchase = true">
          + Buy wholesale voucher batch
        </BaseButton>
      </template>
    </PageHeader>

    <Tabs
      v-model="tab"
      :tabs="[
        { key: 'pins', label: 'My Voucher Inventory', icon: 'ticket', count: data.vouchers.length },
        { key: 'batches', label: 'Wholesale Batches', icon: 'cube', count: data.batches.length },
      ]"
    />

    <BaseCard v-if="tab === 'pins'" :pad="false">
      <DataTable
        :columns="[
          { key: 'serial', label: 'Serial' },
          { key: 'code', label: 'PIN Code' },
          { key: 'planName', label: 'Bundle' },
          { key: 'price', label: 'Retail', align: 'right' },
          { key: 'commission', label: 'Margin', align: 'right' },
          { key: 'status', label: 'Status' },
          { key: 'actions', label: 'Slip', align: 'right' },
        ]"
        :rows="data.vouchers"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'serial'">
            <code class="font-mono text-xs text-ink-faint">{{ row.serial }}</code>
          </template>
          <template v-else-if="col.key === 'code'">
            <code class="rounded bg-canvas px-2 py-0.5 font-mono text-xs font-bold text-ink">{{ row.code }}</code>
          </template>
          <template v-else-if="col.key === 'price'">{{ formatCurrency(row.price) }}</template>
          <template v-else-if="col.key === 'commission'">
            <span class="font-bold text-emerald-600">+{{ formatCurrency(row.commission) }}</span>
          </template>
          <template v-else-if="col.key === 'status'">
            <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
          </template>
          <template v-else-if="col.key === 'actions'">
            <BaseButton size="sm" variant="secondary" icon="printer" @click="selectedVoucher = row">
              QR Slip
            </BaseButton>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <BaseCard v-else :pad="false">
      <DataTable
        :columns="[
          { key: 'batchCode', label: 'Batch Code' },
          { key: 'planName', label: 'Plan' },
          { key: 'zoneName', label: 'Zone' },
          { key: 'quantity', label: 'Total PINs', align: 'right' },
          { key: 'assignedCount', label: 'Unsold Ready', align: 'right' },
          { key: 'wholesalePrice', label: 'Unit Cost', align: 'right' },
        ]"
        :rows="data.batches"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'batchCode'">
            <code class="font-mono text-xs font-bold text-ink">{{ row.batchCode }}</code>
          </template>
          <template v-else-if="col.key === 'wholesalePrice'">{{ formatCurrency(row.wholesalePrice) }}</template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <ModalShell :open="showPurchase" title="Purchase Wholesale Voucher Stock" subtitle="Instant reseller discount applied from your tier rate" @close="showPurchase = false">
      <div class="space-y-4">
        <FormField label="Internet Plan" required>
          <SelectInput
            v-model="form.planId"
            :options="data.plans.map((p: any) => ({ value: p.id, label: `${p.name} (Retail ${formatCurrency(p.price)})` }))"
          />
        </FormField>
        <FormField label="Zone">
          <SelectInput v-model="form.zoneId" :options="data.zones.map((z: any) => ({ value: z.id, label: z.name }))" />
        </FormField>
        <FormField label="Quantity of PINs" required>
          <TextInput v-model="form.quantity" type="number" min="5" max="100" />
        </FormField>
        <div class="rounded-xl border border-line bg-canvas/60 p-3.5 text-xs tnum">
          <div class="flex justify-between">
            <span class="text-ink-faint">Wholesale Float Cost ({{ Math.round(data.agent.commissionRate * 100) }}% discount)</span>
            <strong class="text-base text-ink">{{ formatCurrency(wholesaleEstimate) }}</strong>
          </div>
        </div>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showPurchase = false">Cancel</BaseButton>
        <BaseButton size="sm" @click="buyStockBatch">Purchase Stock</BaseButton>
      </template>
    </ModalShell>

    <VoucherSlipModal
      :open="Boolean(selectedVoucher)"
      :voucher="selectedVoucher"
      @close="selectedVoucher = null"
    />
  </div>
</template>
