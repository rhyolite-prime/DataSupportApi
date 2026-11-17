<script setup lang="ts">
import type { MyNetAgent, MyNetAgentLedgerEntry, MyNetZone } from '@mynet-shared/types/models'

useHead({ title: 'Reseller Agents & Wallets' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const tab = ref('agents')
const agents = ref<MyNetAgent[]>([])
const ledger = ref<MyNetAgentLedgerEntry[]>([])
const zones = ref<MyNetZone[]>([])

const showCreate = ref(false)
const walletModal = ref<{ agent: MyNetAgent; action: 'topup' | 'settle' } | null>(null)
const walletAmount = ref(200)
const walletProvider = ref('MOMO_MTN')

const newAgent = reactive({
  name: '',
  msisdn: '',
  email: '',
  tier: 'STANDARD',
  commissionRate: 0.15,
  territoryZoneId: '',
  openingFloat: 250,
})

async function load() {
  const [aRes, zRes] = await Promise.all([
    api.get<{ agents: MyNetAgent[]; ledger: MyNetAgentLedgerEntry[] }>('/admin/agents'),
    api.get<{ zones: MyNetZone[] }>('/admin/zones'),
  ])
  agents.value = aRes.agents
  ledger.value = aRes.ledger
  zones.value = zRes.zones
  if (!newAgent.territoryZoneId && zRes.zones[0]) newAgent.territoryZoneId = zRes.zones[0].id
}

await load()

async function createAgent() {
  try {
    await api.post('/admin/agents', { ...newAgent })
    toast.success('Agent onboarded', `${newAgent.name} provisioned with TurboLedger float wallet.`)
    showCreate.value = false
    Object.assign(newAgent, { name: '', msisdn: '', email: '' })
    await load()
  } catch (e: any) {
    toast.error('Onboarding failed', e.message)
  }
}

async function submitWalletAction() {
  if (!walletModal.value) return
  try {
    await api.post(`/admin/agents/${walletModal.value.agent.id}/${walletModal.value.action}`, {
      amount: walletAmount.value,
      provider: walletProvider.value,
    })
    toast.success(
      walletModal.value.action === 'topup' ? 'Float credited' : 'MoMo payout settled',
      `${formatCurrency(walletAmount.value)} processed for ${walletModal.value.agent.name}.`,
    )
    walletModal.value = null
    await load()
  } catch (e: any) {
    toast.error('Wallet transaction failed', e.message)
  }
}

async function toggleAgentStatus(agent: MyNetAgent) {
  try {
    await api.post(`/admin/agents/${agent.id}/toggle-status`)
    toast.info('Agent status updated')
    await load()
  } catch (e: any) {
    toast.error('Status update failed', e.message)
  }
}
</script>

<template>
  <div class="space-y-6">
    <PageHeader
      title="Reseller Agents, Float Wallets & Settlements"
      subtitle="Manage field kiosk agents, commission tiers, TurboLedger float accounts, and MoMo commission payouts."
      icon="wallet"
    >
      <template #actions>
        <BaseButton size="sm" icon="plus" @click="showCreate = true">
          + Onboard reseller agent
        </BaseButton>
      </template>
    </PageHeader>

    <Tabs
      v-model="tab"
      :tabs="[
        { key: 'agents', label: 'Field Resellers', icon: 'users', count: agents.length },
        { key: 'ledger', label: 'Agent Wallet Ledger (magma_agent_ledger)', icon: 'banknotes', count: ledger.length },
      ]"
    />

    <div v-if="tab === 'agents'" class="grid gap-5 lg:grid-cols-3">
      <BaseCard v-for="a in agents" :key="a.id">
        <div class="flex items-start justify-between gap-2">
          <div>
            <div class="flex items-center gap-2">
              <code class="rounded bg-canvas px-1.5 py-0.5 font-mono text-[11px] font-bold text-ink">{{ a.agentCode }}</code>
              <Badge :tone="statusMeta(a.tier).tone" size="sm">{{ a.tier }}</Badge>
              <Badge :tone="statusMeta(a.status).tone" size="sm" dot>{{ statusMeta(a.status).label }}</Badge>
            </div>
            <h3 class="mt-2 text-base font-bold text-ink">{{ a.name }}</h3>
            <p class="text-xs text-ink-faint">{{ a.msisdn }} · {{ a.territoryZoneName }}</p>
          </div>
          <span class="rounded-xl bg-primary-soft px-2.5 py-1 text-xs font-extrabold text-primary tnum">
            {{ Math.round(a.commissionRate * 100) }}% comm
          </span>
        </div>

        <div class="mt-4 grid grid-cols-2 gap-3 rounded-xl border border-line bg-canvas/50 p-3.5 text-xs tnum">
          <div>
            <span class="text-ink-faint">Float Balance</span>
            <p class="mt-0.5 text-lg font-extrabold" :class="a.floatBalance < 100 ? 'text-amber-600' : 'text-ink'">
              {{ formatCurrency(a.floatBalance) }}
            </p>
          </div>
          <div>
            <span class="text-ink-faint">30d Commissions</span>
            <p class="mt-0.5 text-lg font-extrabold text-emerald-600 dark:text-emerald-400">
              {{ formatCurrency(a.commissionEarnedMonth) }}
            </p>
          </div>
          <div>
            <span class="text-ink-faint">Stocked PINs</span>
            <p class="mt-0.5 font-bold text-ink">{{ a.vouchersInStock }} ready</p>
          </div>
          <div>
            <span class="text-ink-faint">30d Vouchers Sold</span>
            <p class="mt-0.5 font-bold text-ink">{{ a.vouchersSoldMonth }} sold</p>
          </div>
        </div>

        <div class="mt-4 flex flex-wrap items-center justify-between gap-2 border-t border-line pt-3">
          <div class="flex gap-1.5">
            <BaseButton size="sm" variant="secondary" @click="walletModal = { agent: a, action: 'topup' }; walletAmount = 200">
              + Credit Float
            </BaseButton>
            <BaseButton size="sm" variant="secondary" @click="walletModal = { agent: a, action: 'settle' }; walletAmount = 100">
              Settle MoMo
            </BaseButton>
          </div>
          <BaseButton size="sm" variant="ghost" @click="toggleAgentStatus(a)">
            {{ a.status === 'ACTIVE' ? 'Suspend' : 'Activate' }}
          </BaseButton>
        </div>
      </BaseCard>
    </div>

    <BaseCard v-else title="Immutable Agent Wallet Statement Chain" :pad="false">
      <DataTable
        :columns="[
          { key: 'reference', label: 'Reference' },
          { key: 'agentName', label: 'Agent' },
          { key: 'entryType', label: 'Entry Type' },
          { key: 'description', label: 'Description' },
          { key: 'amount', label: 'Movement', align: 'right' },
          { key: 'balanceAfter', label: 'Balance After', align: 'right' },
          { key: 'createdAt', label: 'Timestamp' },
        ]"
        :rows="ledger"
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

    <!-- Wallet Credit / Settlement Modal -->
    <ModalShell
      :open="Boolean(walletModal)"
      :title="walletModal?.action === 'topup' ? `Credit Float · ${walletModal?.agent.name}` : `MoMo Settlement · ${walletModal?.agent.name}`"
      @close="walletModal = null"
    >
      <div class="space-y-4">
        <FormField label="Amount (GH₵)" required>
          <TextInput v-model="walletAmount" type="number" min="10" step="5" />
        </FormField>
        <FormField label="Payment Rail">
          <SelectInput
            v-model="walletProvider"
            :options="[
              { value: 'MOMO_MTN', label: 'MTN Mobile Money' },
              { value: 'MOMO_TELECEL', label: 'Telecel Cash' },
              { value: 'MOMO_AIRTELTIGO', label: 'AirtelTigo Money' },
              { value: 'BANK_TRANSFER', label: 'Bank Deposit' },
            ]"
          />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="walletModal = null">Cancel</BaseButton>
        <BaseButton size="sm" @click="submitWalletAction">
          {{ walletModal?.action === 'topup' ? 'Credit Wallet' : 'Send Payout' }}
        </BaseButton>
      </template>
    </ModalShell>

    <!-- Onboard Agent Modal -->
    <ModalShell :open="showCreate" title="Onboard New MyNet Reseller Agent" width="max-w-xl" @close="showCreate = false">
      <div class="grid gap-4 sm:grid-cols-2">
        <FormField label="Shop / Agent Name" required>
          <TextInput v-model="newAgent.name" placeholder="Osu Oxford Kiosk" />
        </FormField>
        <FormField label="Agent MoMo Phone" required>
          <TextInput v-model="newAgent.msisdn" placeholder="0244667788" />
        </FormField>
        <FormField label="Territory Zone">
          <SelectInput v-model="newAgent.territoryZoneId" :options="zones.map(z => ({ value: z.id, label: z.name }))" />
        </FormField>
        <FormField label="Reseller Tier">
          <SelectInput
            v-model="newAgent.tier"
            :options="[
              { value: 'STANDARD', label: 'Standard (12%)' },
              { value: 'PREMIER', label: 'Premier (15%)' },
              { value: 'SUPER', label: 'Super Agent (18%)' },
            ]"
          />
        </FormField>
        <FormField label="Opening Float Deposit (GH₵)">
          <TextInput v-model="newAgent.openingFloat" type="number" min="0" />
        </FormField>
        <FormField label="Email Address">
          <TextInput v-model="newAgent.email" type="email" placeholder="agent@mynet.gh" />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showCreate = false">Cancel</BaseButton>
        <BaseButton size="sm" @click="createAgent">Create Agent</BaseButton>
      </template>
    </ModalShell>
  </div>
</template>
