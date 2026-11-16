<script setup lang="ts">
import type { MyNetVoucher } from '@mynet-shared/types/models'

useHead({ title: 'Reseller Overview' })

const api = useMyNetApi()
const { statusMeta } = useStatusMeta()
const agentFloat = useState<number>('mynet-agent-float-badge', () => 0)

const data = ref<any>(null)
const selectedVoucher = ref<MyNetVoucher | null>(null)

async function load() {
  data.value = await api.get('/agent/dashboard')
  if (data.value?.agent) {
    agentFloat.value = data.value.agent.floatBalance
  }
}

await load()

const salesChartOption = computed(() => {
  const rows = data.value?.dailySalesChart || []
  return {
    tooltip: { trigger: 'axis' },
    legend: { bottom: 0, textStyle: { fontSize: 11 } },
    grid: { left: 42, right: 16, top: 18, bottom: 38 },
    xAxis: { type: 'category', data: rows.map((r: any) => r.day) },
    yAxis: { type: 'value', name: 'GH₵' },
    series: [
      {
        name: 'Retail Voucher Sales',
        type: 'bar',
        barMaxWidth: 26,
        itemStyle: { color: '#4338CA', borderRadius: [6, 6, 0, 0] },
        data: rows.map((r: any) => r.sales),
      },
      {
        name: 'Commission Earned',
        type: 'line',
        smooth: true,
        itemStyle: { color: '#F97316' },
        lineStyle: { width: 3 },
        data: rows.map((r: any) => r.commission),
      },
    ],
  }
})
</script>

<template>
  <div v-if="data" class="space-y-6">
    <PageHeader
      :title="`Welcome back, ${data.agent.name}`"
      :subtitle="`${data.agent.agentCode} · ${data.agent.tier} Reseller (${Math.round(data.agent.commissionRate * 100)}% commission) · ${data.territoryZone.name}`"
      icon="squares-2x2"
    >
      <template #actions>
        <NuxtLink to="/topup">
          <BaseButton variant="secondary" size="sm" icon="device-phone-mobile">
            Direct Subscriber Top-Up
          </BaseButton>
        </NuxtLink>
        <NuxtLink to="/sell">
          <BaseButton size="sm" icon="bolt">
            Sell Instant Voucher
          </BaseButton>
        </NuxtLink>
      </template>
    </PageHeader>

    <!-- KPI Row -->
    <div class="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
      <StatCard
        label="Available Float Wallet"
        :value="formatCurrency(data.agent.floatBalance)"
        :sub="`Wallet ${data.agent.walletAccountId}`"
        icon="wallet"
        :tone="data.agent.floatBalance < 120 ? 'amber' : 'primary'"
      />
      <StatCard
        label="30-Day Commission Earned"
        :value="formatCurrency(data.agent.commissionEarnedMonth)"
        :sub="`${Math.round(data.agent.commissionRate * 100)}% margin on every bundle`"
        icon="banknotes"
        tone="green"
      />
      <StatCard
        label="Pre-Assigned Stock PINs"
        :value="data.agent.vouchersInStock"
        sub="Ready for instant offline or POS sale"
        icon="ticket"
        tone="purple"
      />
      <StatCard
        label="30-Day Vouchers Sold"
        :value="data.agent.vouchersSoldMonth"
        :sub="`Home zone: ${data.territoryZone.status} (${data.territoryZone.activeSessions} online)`"
        icon="wifi"
        tone="blue"
      />
    </div>

    <!-- Quick Bundle Stock + Weekly Chart -->
    <div class="grid gap-6 lg:grid-cols-12">
      <BaseCard
        class="lg:col-span-5"
        title="Fast-Moving Bundles & Stock"
        subtitle="Tap any bundle to launch Instant POS"
        icon="bolt"
      >
        <div class="space-y-2.5">
          <NuxtLink
            v-for="item in data.stockByPlan"
            :key="item.planId"
            :to="`/sell?plan=${item.planCode}`"
            class="flex items-center justify-between rounded-xl border border-line bg-canvas/50 px-3.5 py-2.5 text-xs transition hover:border-primary/40 hover:bg-primary-soft/30"
          >
            <div>
              <p class="font-bold text-ink">{{ item.planName }}</p>
              <p class="text-[11px] text-ink-faint tnum">
                Retail {{ formatCurrency(item.price) }} · Earn <strong class="text-emerald-600">+{{ formatCurrency(item.commission) }}</strong>
              </p>
            </div>
            <Badge :tone="item.inStock > 0 ? 'green' : 'gray'" size="sm">
              {{ item.inStock }} stocked
            </Badge>
          </NuxtLink>
        </div>
      </BaseCard>

      <BaseCard
        class="lg:col-span-7"
        title="7-Day Kiosk Sales & Commission"
        subtitle="Daily retail sales volume vs reseller commission earned (GH₵)"
        icon="chart-bar-square"
      >
        <ClientOnly>
          <EChart :option="salesChartOption" height="275px" />
        </ClientOnly>
      </BaseCard>
    </div>

    <!-- Recent Sales Receipt Log -->
    <BaseCard title="Recent Voucher Sales & Reprints" subtitle="Click Reprint Slip to display QR code or resend PIN to customer" :pad="false">
      <DataTable
        :columns="[
          { key: 'serial', label: 'Receipt / Serial' },
          { key: 'code', label: 'Voucher PIN' },
          { key: 'planName', label: 'Bundle' },
          { key: 'customerName', label: 'Customer' },
          { key: 'price', label: 'Retail', align: 'right' },
          { key: 'commission', label: 'Earned', align: 'right' },
          { key: 'status', label: 'State' },
          { key: 'actions', label: 'Receipt', align: 'right' },
        ]"
        :rows="data.recentSales"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'serial'">
            <code class="font-mono text-xs font-semibold text-ink">{{ row.serial }}</code>
          </template>
          <template v-else-if="col.key === 'code'">
            <code class="rounded bg-canvas px-2 py-0.5 font-mono text-xs font-bold text-primary">{{ row.code }}</code>
          </template>
          <template v-else-if="col.key === 'customerName'">
            <span class="font-medium text-ink">{{ row.customerName || 'Walk-in' }}</span>
            <span v-if="row.customerMsisdn" class="block text-[11px] text-ink-faint tnum">{{ row.customerMsisdn }}</span>
          </template>
          <template v-else-if="col.key === 'price'">
            <span class="font-bold text-ink">{{ formatCurrency(row.price) }}</span>
          </template>
          <template v-else-if="col.key === 'commission'">
            <span class="font-bold text-emerald-600">+{{ formatCurrency(row.commission) }}</span>
          </template>
          <template v-else-if="col.key === 'status'">
            <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
          </template>
          <template v-else-if="col.key === 'actions'">
            <BaseButton size="sm" variant="secondary" icon="printer" @click="selectedVoucher = row">
              Reprint Slip
            </BaseButton>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <VoucherSlipModal
      :open="Boolean(selectedVoucher)"
      :voucher="selectedVoucher"
      @close="selectedVoucher = null"
    />
  </div>
</template>
