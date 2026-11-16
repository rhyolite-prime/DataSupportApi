<script setup lang="ts">
useHead({ title: 'Billing, Revenue & Tax' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const tab = ref('transactions')
const data = ref<any>(null)
const runningInvoices = ref(false)

async function load() {
  data.value = await api.get('/admin/billing')
}

await load()

async function txAction(id: string, action: 'verify' | 'refund') {
  try {
    await api.post(`/admin/transactions/${id}/${action}`)
    toast.success(action === 'verify' ? 'Payment verified' : 'Transaction refunded')
    await load()
  } catch (e: any) {
    toast.error('Operation failed', e.message)
  }
}

async function runInvoiceCycle() {
  runningInvoices.value = true
  try {
    const res = await api.post<any>('/admin/invoices/run')
    toast.success('Invoices generated', `Issued ${res.generated} monthly PPPoE invoice(s).`)
    await load()
  } catch (e: any) {
    toast.error('Billing run failed', e.message)
  } finally {
    runningInvoices.value = false
  }
}

function exportTaxPack() {
  if (!import.meta.client || !data.value) return
  const rows = data.value.transactions.filter((t: any) => t.status === 'SUCCESS')
  const csv = [
    'Reference,Payer,MSISDN,Method,GrossGHS,NetGHS,VAT15,NHIL2_5,GETFund2_5,PaidAt',
    ...rows.map((r: any) =>
      `${r.internalReference},"${r.subscriberName}",${r.payerMsisdn},${r.paymentMethod},${r.amount.toFixed(2)},${r.netAmount.toFixed(2)},${r.taxBreakdown.vat.toFixed(2)},${r.taxBreakdown.nhil.toFixed(2)},${r.taxBreakdown.getFund.toFixed(2)},${r.paidAt || ''}`,
    ),
  ].join('\n')
  const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = 'MyNet-GRA-Tax-Schedule.csv'
  a.click()
  URL.revokeObjectURL(url)
  toast.success('GRA Tax CSV downloaded')
}
</script>

<template>
  <div v-if="data" class="space-y-6">
    <PageHeader
      title="Billing, Revenue Reconciliation & Ghana Tax Pack"
      subtitle="Verification-first Hubtel MoMo & Paystack callbacks, residential PPPoE invoicing, and statutory VAT/NHIL/GETFund schedules."
      icon="banknotes"
    >
      <template #actions>
        <BaseButton variant="secondary" size="sm" icon="arrow-down-tray" @click="exportTaxPack">
          Export GRA Tax CSV
        </BaseButton>
        <BaseButton size="sm" icon="bolt" :loading="runningInvoices" @click="runInvoiceCycle">
          Run Monthly PPPoE Invoices
        </BaseButton>
      </template>
    </PageHeader>

    <div class="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
      <StatCard label="Verified Gross Collections" :value="formatCurrency(data.summary.grossRevenue)" sub="Tax-inclusive settled receipts" icon="banknotes" tone="green" />
      <StatCard label="Net Operating Revenue" :value="formatCurrency(data.summary.netRevenue)" sub="Ex-statutory levies" icon="scale" tone="primary" />
      <StatCard label="GRA Statutory Tax Liability" :value="formatCurrency(data.summary.totalTax)" :sub="`VAT ${formatCurrency(data.summary.vat)} · NHIL/GETFund ${formatCurrency(data.summary.nhil + data.summary.getFund)}`" icon="document-text" tone="purple" />
      <StatCard label="Pending / Overdue Items" :value="data.summary.pendingCount + data.summary.overdueInvoices" :sub="`${data.summary.pendingCount} pending MoMo · ${data.summary.overdueInvoices} overdue invoice`" icon="clock" tone="amber" />
    </div>

    <Tabs
      v-model="tab"
      :tabs="[
        { key: 'transactions', label: 'Payment Transactions', icon: 'credit-card', count: data.transactions.length },
        { key: 'invoices', label: 'PPPoE Monthly Invoices', icon: 'document-text', count: data.invoices.length },
        { key: 'tax', label: 'Statutory Tax Breakdown', icon: 'scale' },
      ]"
    />

    <BaseCard v-if="tab === 'transactions'" :pad="false">
      <DataTable
        :columns="[
          { key: 'internalReference', label: 'Ref' },
          { key: 'subscriberName', label: 'Subscriber / Payer' },
          { key: 'planCode', label: 'Plan / Purpose' },
          { key: 'paymentMethod', label: 'Method' },
          { key: 'amount', label: 'Gross', align: 'right' },
          { key: 'taxAmount', label: 'Tax', align: 'right' },
          { key: 'status', label: 'Status' },
          { key: 'actions', label: 'Reconcile', align: 'right' },
        ]"
        :rows="data.transactions"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'internalReference'">
            <code class="font-mono text-xs font-semibold text-ink">{{ row.internalReference }}</code>
            <div class="text-[10px] text-ink-faint">{{ row.provider }}</div>
          </template>
          <template v-else-if="col.key === 'subscriberName'">
            <div class="font-medium text-ink">{{ row.subscriberName }}</div>
            <div class="text-[11px] text-ink-faint tnum">{{ row.payerMsisdn }}</div>
          </template>
          <template v-else-if="col.key === 'planCode'">
            <span class="font-semibold text-ink">{{ row.planCode }}</span>
            <span class="ml-1 text-xs text-ink-faint">({{ row.purpose }})</span>
          </template>
          <template v-else-if="col.key === 'amount'">
            <span class="font-bold text-ink">{{ formatCurrency(row.amount) }}</span>
          </template>
          <template v-else-if="col.key === 'taxAmount'">
            <span class="text-xs text-ink-faint">{{ formatCurrency(row.taxAmount) }}</span>
          </template>
          <template v-else-if="col.key === 'status'">
            <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
          </template>
          <template v-else-if="col.key === 'actions'">
            <div class="flex justify-end gap-1.5">
              <BaseButton v-if="row.status === 'PENDING'" size="sm" variant="success" @click="txAction(row.id, 'verify')">
                Verify
              </BaseButton>
              <BaseButton v-if="row.status === 'SUCCESS'" size="sm" variant="ghost" @click="txAction(row.id, 'refund')">
                Refund
              </BaseButton>
            </div>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <BaseCard v-else-if="tab === 'invoices'" :pad="false">
      <DataTable
        :columns="[
          { key: 'invoiceNumber', label: 'Invoice #' },
          { key: 'subscriberName', label: 'Subscriber' },
          { key: 'planName', label: 'Plan' },
          { key: 'periodStart', label: 'Billing Period' },
          { key: 'total', label: 'Total', align: 'right' },
          { key: 'status', label: 'Status' },
        ]"
        :rows="data.invoices"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'invoiceNumber'">
            <code class="font-mono text-xs font-bold text-ink">{{ row.invoiceNumber }}</code>
          </template>
          <template v-else-if="col.key === 'subscriberName'">
            <div class="font-medium text-ink">{{ row.subscriberName }}</div>
            <div class="text-[11px] text-ink-faint">{{ row.msisdn }} · {{ row.zoneName }}</div>
          </template>
          <template v-else-if="col.key === 'periodStart'">
            <span class="text-xs text-ink-faint">{{ row.periodStart }} → {{ row.periodEnd }}</span>
          </template>
          <template v-else-if="col.key === 'total'">
            <span class="font-bold text-ink">{{ formatCurrency(row.total) }}</span>
          </template>
          <template v-else-if="col.key === 'status'">
            <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <BaseCard v-else title="Ghana Revenue Authority (GRA) Statutory Levy Split" subtitle="Computed automatically per transaction in MagmaBillingService">
      <div class="grid gap-4 sm:grid-cols-3 tnum">
        <div class="rounded-xl border border-line bg-canvas/50 p-4">
          <p class="text-xs text-ink-faint">Value Added Tax (VAT · 15%)</p>
          <p class="mt-1 text-2xl font-extrabold text-ink">{{ formatCurrency(data.summary.vat) }}</p>
        </div>
        <div class="rounded-xl border border-line bg-canvas/50 p-4">
          <p class="text-xs text-ink-faint">NHIL Levy (2.5%)</p>
          <p class="mt-1 text-2xl font-extrabold text-ink">{{ formatCurrency(data.summary.nhil) }}</p>
        </div>
        <div class="rounded-xl border border-line bg-canvas/50 p-4">
          <p class="text-xs text-ink-faint">GETFund Levy (2.5%)</p>
          <p class="mt-1 text-2xl font-extrabold text-ink">{{ formatCurrency(data.summary.getFund) }}</p>
        </div>
      </div>
    </BaseCard>
  </div>
</template>
