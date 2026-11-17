<script setup lang="ts">
useHead({ title: 'Float Wallet & MoMo Payout' })

const api = useMyNetApi()
const toast = useToast()
const agentFloat = useState<number>('mynet-agent-float-badge', () => 0)

const data = ref<any>(null)
const modalMode = ref<'topup' | 'settlement' | null>(null)
const amount = ref(150)
const provider = ref('MOMO_MTN')
const msisdn = ref('')

async function load() {
  data.value = await api.get('/agent/wallet')
  if (data.value?.agent) {
    agentFloat.value = data.value.agent.floatBalance
    if (!msisdn.value) msisdn.value = data.value.agent.msisdn
  }
}

await load()

async function submitWallet() {
  if (!modalMode.value) return
  try {
    await api.post(`/agent/wallet/${modalMode.value}`, {
      amount: amount.value,
      provider: provider.value,
      msisdn: msisdn.value,
    })
    toast.success(
      modalMode.value === 'topup' ? 'Float wallet funded' : 'MoMo payout dispatched',
      `${formatCurrency(amount.value)} processed via ${provider.value}.`,
    )
    modalMode.value = null
    await load()
  } catch (e: any) {
    toast.error('Wallet transaction failed', e.message)
  }
}
</script>

<template>
  <div v-if="data" class="space-y-6">
    <PageHeader
      title="Reseller Float Wallet & MoMo Settlements"
      subtitle="Fund your MyNet float via Mobile Money or withdraw accumulated reseller commissions."
      icon="wallet"
    >
      <template #actions>
        <BaseButton variant="secondary" size="sm" icon="banknotes" @click="modalMode = 'settlement'; amount = 100">
          Withdraw to MoMo
        </BaseButton>
        <BaseButton size="sm" icon="plus" @click="modalMode = 'topup'; amount = 200">
          + Top Up Float via MoMo
        </BaseButton>
      </template>
    </PageHeader>

    <div class="grid gap-4 sm:grid-cols-3">
      <StatCard label="Available TurboLedger Float" :value="formatCurrency(data.agent.floatBalance)" :sub="`Account ${data.agent.walletAccountId}`" icon="wallet" tone="primary" />
      <StatCard label="30-Day Commissions Earned" :value="formatCurrency(data.agent.commissionEarnedMonth)" :sub="`${data.agent.tier} Tier (${Math.round(data.agent.commissionRate * 100)}%)`" icon="banknotes" tone="green" />
      <StatCard label="Registered Settlement MoMo" :value="data.agent.msisdn" :sub="data.agent.territoryZoneName" icon="device-phone-mobile" tone="purple" />
    </div>

    <BaseCard title="Wallet Statement & Ledger History" :pad="false">
      <DataTable
        :columns="[
          { key: 'reference', label: 'Reference' },
          { key: 'entryType', label: 'Type' },
          { key: 'description', label: 'Narration' },
          { key: 'amount', label: 'Amount', align: 'right' },
          { key: 'balanceAfter', label: 'Balance After', align: 'right' },
          { key: 'createdAt', label: 'Timestamp' },
        ]"
        :rows="data.ledger"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'reference'">
            <code class="font-mono text-xs font-semibold text-ink">{{ row.reference }}</code>
          </template>
          <template v-else-if="col.key === 'entryType'">
            <Badge :tone="row.amount >= 0 ? 'green' : 'blue'" size="sm">{{ row.entryType }}</Badge>
          </template>
          <template v-else-if="col.key === 'amount'">
            <span class="font-bold" :class="row.amount >= 0 ? 'text-emerald-600' : 'text-ink'">
              {{ row.amount >= 0 ? '+' : '' }}{{ formatCurrency(row.amount) }}
            </span>
          </template>
          <template v-else-if="col.key === 'balanceAfter'">
            <span class="font-bold text-ink">{{ formatCurrency(row.balanceAfter) }}</span>
          </template>
          <template v-else-if="col.key === 'createdAt'">
            <span class="text-xs text-ink-faint">{{ formatDateTime(row.createdAt) }}</span>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <ModalShell
      :open="Boolean(modalMode)"
      :title="modalMode === 'topup' ? 'Top Up Reseller Float via Mobile Money' : 'Withdraw Commission / Float to MoMo'"
      @close="modalMode = null"
    >
      <div class="space-y-4">
        <FormField label="Amount (GH₵)" required>
          <TextInput v-model="amount" type="number" min="10" step="5" />
        </FormField>
        <FormField label="Mobile Money Network">
          <SelectInput
            v-model="provider"
            :options="[
              { value: 'MOMO_MTN', label: 'MTN Mobile Money' },
              { value: 'MOMO_TELECEL', label: 'Telecel Cash' },
              { value: 'MOMO_AIRTELTIGO', label: 'AirtelTigo Money' },
            ]"
          />
        </FormField>
        <FormField label="MoMo Wallet Phone Number" required>
          <TextInput v-model="msisdn" placeholder="0244555666" />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="modalMode = null">Cancel</BaseButton>
        <BaseButton size="sm" @click="submitWallet">
          {{ modalMode === 'topup' ? 'Approve MoMo Prompt' : 'Send Payout' }}
        </BaseButton>
      </template>
    </ModalShell>
  </div>
</template>
