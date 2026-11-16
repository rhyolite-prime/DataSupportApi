<script setup lang="ts">
import type { MyNetVoucher } from '@mynet-shared/types/models'

useHead({ title: 'Instant Voucher POS' })

const route = useRoute()
const api = useMyNetApi()
const toast = useToast()
const { queue, simulateOffline, enqueueSale, removeQueued, syncAll } = useOfflineSalesQueue()
const agentFloat = useState<number>('mynet-agent-float-badge', () => 0)

const catalog = ref<any>(null)
const selectedPlanId = ref('')
const selectedZoneId = ref('')
const source = ref<'stock' | 'float'>('stock')
const customerMsisdn = ref('')
const customerName = ref('')
const issuing = ref(false)

const issuedVoucher = ref<MyNetVoucher | null>(null)
const issuedRedeemUrl = ref('')

async function load() {
  catalog.value = await api.get('/agent/catalog')
  if (catalog.value?.agent) {
    agentFloat.value = catalog.value.agent.floatBalance
    if (!selectedZoneId.value) selectedZoneId.value = catalog.value.agent.territoryZoneId
  }
  const queryPlan = String(route.query.plan || '')
  if (queryPlan && catalog.value?.plans) {
    const match = catalog.value.plans.find((p: any) => p.planCode === queryPlan)
    if (match) selectedPlanId.value = match.id
  }
  if (!selectedPlanId.value && catalog.value?.plans?.[1]) {
    selectedPlanId.value = catalog.value.plans[1].id
  }
}

await load()

const activePlan = computed(() =>
  catalog.value?.plans?.find((p: any) => p.id === selectedPlanId.value) || catalog.value?.plans?.[0],
)

watch(
  activePlan,
  (p) => {
    if (!p) return
    source.value = p.inStock > 0 ? 'stock' : 'float'
  },
  { immediate: true },
)

async function issueNow() {
  if (!activePlan.value || issuing.value) return

  if (simulateOffline.value) {
    enqueueSale({
      planId: activePlan.value.id,
      planCode: activePlan.value.planCode,
      planName: activePlan.value.name,
      price: activePlan.value.price,
      zoneId: selectedZoneId.value,
      source: source.value,
      customerMsisdn: customerMsisdn.value,
      customerName: customerName.value || 'Walk-in Customer',
    })
    toast.warning('Queued offline', `${activePlan.value.planCode} saved to offline POS queue for one-tap sync.`)
    customerMsisdn.value = ''
    customerName.value = ''
    return
  }

  issuing.value = true
  try {
    const res = await api.post<{ voucher: MyNetVoucher; portalRedeemUrl: string }>('/agent/sell', {
      planId: activePlan.value.id,
      zoneId: selectedZoneId.value,
      source: source.value,
      customerMsisdn: customerMsisdn.value,
      customerName: customerName.value || 'Walk-in Customer',
    })
    issuedVoucher.value = res.voucher
    issuedRedeemUrl.value = res.portalRedeemUrl
    customerMsisdn.value = ''
    customerName.value = ''
    toast.success('Voucher PIN minted', `${res.voucher.code} ready to print or scan.`)
    await load()
  } catch (e: any) {
    toast.error('Sale could not be completed', e.message)
  } finally {
    issuing.value = false
  }
}

async function flushOfflineQueue() {
  const res = await syncAll(api)
  if (res.lastResult?.voucher) {
    issuedVoucher.value = res.lastResult.voucher
    issuedRedeemUrl.value = res.lastResult.portalRedeemUrl || ''
  }
  toast.success('Offline queue synced', `${res.syncedCount} queued sale(s) issued.`)
  await load()
}
</script>

<template>
  <div v-if="catalog" class="space-y-6">
    <PageHeader
      title="Instant WiFi Voucher Point-of-Sale"
      subtitle="Select a MyNet internet bundle, collect cash or MoMo from the walk-in customer, and issue an instant QR/PIN slip."
      icon="bolt"
    />

    <div class="grid gap-6 lg:grid-cols-12">
      <!-- Bundle Grid -->
      <div class="space-y-4 lg:col-span-7">
        <div class="grid gap-3.5 sm:grid-cols-2">
          <button
            v-for="p in catalog.plans"
            :key="p.id"
            type="button"
            class="cursor-pointer rounded-2xl border p-4 text-left transition"
            :class="
              selectedPlanId === p.id
                ? 'border-primary bg-primary-soft/50 ring-2 ring-primary/20'
                : 'border-line bg-surface hover:border-primary/40'
            "
            @click="selectedPlanId = p.id"
          >
            <div class="flex items-center justify-between gap-2">
              <code class="rounded bg-canvas px-2 py-0.5 font-mono text-[11px] font-bold text-ink">{{ p.planCode }}</code>
              <Badge :tone="p.inStock > 0 ? 'green' : 'blue'" size="sm">
                {{ p.inStock > 0 ? `${p.inStock} in stock` : 'Instant Float' }}
              </Badge>
            </div>
            <h3 class="mt-2.5 text-base font-bold text-ink">{{ p.name }}</h3>
            <p class="mt-0.5 text-xs text-ink-faint">
              {{ p.dataCapMb ? formatDataVolume(p.dataCapMb) : 'Unlimited Fair-Use' }} · {{ formatSpeedKbps(p.downKbps) }} · {{ p.deviceLimit }} device(s)
            </p>
            <div class="mt-4 flex items-baseline justify-between border-t border-line pt-3 tnum">
              <span class="text-xl font-extrabold text-ink">{{ formatCurrency(p.price) }}</span>
              <span class="text-xs font-bold text-emerald-600 dark:text-emerald-400">
                Earn +{{ formatCurrency(p.effectiveCommission) }}
              </span>
            </div>
          </button>
        </div>
      </div>

      <!-- Checkout Ticket Card -->
      <div class="space-y-4 lg:col-span-5">
        <BaseCard title="POS Checkout & Fulfillment" subtitle="Choose fulfillment source and optional SMS delivery" icon="ticket">
          <div v-if="activePlan" class="space-y-4">
            <div class="grid grid-cols-2 gap-1.5 rounded-xl bg-canvas p-1 text-xs">
              <button
                type="button"
                class="cursor-pointer rounded-lg py-2 font-semibold transition"
                :class="source === 'stock' ? 'bg-surface text-ink shadow-xs' : 'text-ink-faint'"
                @click="source = 'stock'"
              >
                Pre-Stocked ({{ activePlan.inStock }})
              </button>
              <button
                type="button"
                class="cursor-pointer rounded-lg py-2 font-semibold transition"
                :class="source === 'float' ? 'bg-surface text-ink shadow-xs' : 'text-ink-faint'"
                @click="source = 'float'"
              >
                Mint from Float
              </button>
            </div>

            <FormField label="Target WiFi Zone">
              <SelectInput
                v-model="selectedZoneId"
                :options="catalog.zones.map((z: any) => ({ value: z.id, label: `${z.name} (${z.zoneCode})` }))"
              />
            </FormField>

            <FormField label="Customer Phone for SMS Copy (Optional)" hint="Leave blank for anonymous thermal/QR slip">
              <TextInput v-model="customerMsisdn" placeholder="0244123456" />
            </FormField>

            <FormField label="Customer Label (Optional)">
              <TextInput v-model="customerName" placeholder="Walk-in Customer" />
            </FormField>

            <div class="space-y-1.5 rounded-xl border border-line bg-canvas/60 p-3.5 text-xs tnum">
              <div class="flex justify-between">
                <span class="text-ink-faint">Collect from Customer (Retail)</span>
                <strong class="text-base text-ink">{{ formatCurrency(activePlan.price) }}</strong>
              </div>
              <div class="flex justify-between">
                <span class="text-ink-faint">Your Reseller Commission</span>
                <strong class="text-emerald-600">+{{ formatCurrency(activePlan.effectiveCommission) }}</strong>
              </div>
              <div class="flex justify-between border-t border-line pt-1.5">
                <span class="text-ink-faint">Net Float Debit (if minted)</span>
                <strong class="text-ink">{{ source === 'stock' && activePlan.inStock > 0 ? 'GH₵ 0.00 (Prepaid Stock)' : formatCurrency(activePlan.netFloatCost) }}</strong>
              </div>
            </div>

            <BaseButton block icon="bolt" :loading="issuing" @click="issueNow">
              {{ simulateOffline ? `Queue ${activePlan.planCode} Sale Offline` : `Issue ${activePlan.planCode} Voucher (${formatCurrency(activePlan.price)})` }}
            </BaseButton>
          </div>
        </BaseCard>

        <!-- Offline Queue Card -->
        <BaseCard
          v-if="queue.length"
          title="Pending Offline POS Queue"
          :subtitle="`${queue.length} sale(s) waiting for backhaul sync`"
          icon="clock"
        >
          <div class="space-y-2 text-xs">
            <div
              v-for="item in queue"
              :key="item.id"
              class="flex items-center justify-between rounded-xl border border-line px-3 py-2"
            >
              <div>
                <p class="font-bold text-ink">{{ item.planName }} ({{ formatCurrency(item.price) }})</p>
                <p class="text-[11px] text-ink-faint">{{ item.customerMsisdn || 'Walk-in' }} · {{ formatRelativeTime(item.queuedAt) }}</p>
              </div>
              <BaseButton size="sm" variant="ghost" @click="removeQueued(item.id)">Remove</BaseButton>
            </div>
            <BaseButton block variant="accent" size="sm" icon="arrow-path" @click="flushOfflineQueue">
              Sync All Queued Sales Now
            </BaseButton>
          </div>
        </BaseCard>
      </div>
    </div>

    <VoucherSlipModal
      :open="Boolean(issuedVoucher)"
      :voucher="issuedVoucher"
      :redeem-url="issuedRedeemUrl"
      @close="issuedVoucher = null"
    />
  </div>
</template>
