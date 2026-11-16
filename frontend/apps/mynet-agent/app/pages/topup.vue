<script setup lang="ts">
import type { MyNetDataPlan, MyNetSubscriber } from '@mynet-shared/types/models'

useHead({ title: 'Direct Subscriber Top-Up' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()
const agentFloat = useState<number>('mynet-agent-float-badge', () => 0)

const query = ref('')
const subscribers = ref<MyNetSubscriber[]>([])
const plans = ref<MyNetDataPlan[]>([])

const msisdn = ref('0244123456')
const fullName = ref('Ama Mensah')
const selectedPlanId = ref('')
const busy = ref(false)

async function searchSubscribers() {
  const res = await api.get<{ subscribers: MyNetSubscriber[]; plans: MyNetDataPlan[] }>('/agent/subscriber-lookup', {
    q: query.value,
  })
  subscribers.value = res.subscribers
  plans.value = res.plans
  if (!selectedPlanId.value && res.plans[1]) selectedPlanId.value = res.plans[1].id
}

await searchSubscribers()

function chooseSubscriber(s: MyNetSubscriber) {
  msisdn.value = s.msisdn
  fullName.value = s.fullName
}

async function activateBundle() {
  busy.value = true
  try {
    const res = await api.post<any>('/agent/subscriber-topup', {
      msisdn: msisdn.value,
      fullName: fullName.value,
      planId: selectedPlanId.value,
    })
    if (res?.agent) agentFloat.value = res.agent.floatBalance
    toast.success('Bundle activated!', `${res.subscriber.activePlanName} active on ${res.subscriber.msisdn} (Earned +${formatCurrency(res.commission)}).`)
    await searchSubscribers()
  } catch (e: any) {
    toast.error('Top-up failed', e.message)
  } finally {
    busy.value = false
  }
}
</script>

<template>
  <div class="space-y-6">
    <PageHeader
      title="Direct Subscriber Lookup & Bundle Top-Up"
      subtitle="Activate Hotspot or residential PPPoE internet bundles directly on a subscriber phone number without printing a PIN."
      icon="device-phone-mobile"
    />

    <div class="grid gap-6 lg:grid-cols-12">
      <BaseCard class="lg:col-span-7" title="Find Existing MyNet Subscriber" subtitle="Click any subscriber to prefill top-up form" :pad="false">
        <div class="border-b border-line p-4">
          <div class="flex gap-2">
            <TextInput v-model="query" placeholder="Search customer phone (0244...) or name…" @keyup.enter="searchSubscribers" />
            <BaseButton variant="secondary" size="sm" @click="searchSubscribers">Search</BaseButton>
          </div>
        </div>
        <DataTable
          clickable
          :columns="[
            { key: 'fullName', label: 'Subscriber' },
            { key: 'authType', label: 'Mode' },
            { key: 'activePlanCode', label: 'Current Plan' },
            { key: 'status', label: 'Status' },
          ]"
          :rows="subscribers"
          @row-click="chooseSubscriber"
        >
          <template #cell="{ col, row }">
            <template v-if="col.key === 'fullName'">
              <p class="font-semibold text-ink">{{ row.fullName }}</p>
              <p class="text-[11px] text-ink-faint tnum">{{ row.msisdn }} · {{ row.homeZoneName }}</p>
            </template>
            <template v-else-if="col.key === 'authType'">
              <Badge :tone="row.authType === 'PPPOE' ? 'purple' : 'blue'" size="sm">{{ row.authType }}</Badge>
            </template>
            <template v-else-if="col.key === 'status'">
              <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
            </template>
            <template v-else>{{ row[col.key] || '—' }}</template>
          </template>
        </DataTable>
      </BaseCard>

      <BaseCard class="lg:col-span-5" title="Instant Direct Activation" subtitle="Debits net wholesale price from your reseller float" icon="bolt">
        <div class="space-y-4">
          <FormField label="Subscriber Phone (MSISDN)" required>
            <TextInput v-model="msisdn" placeholder="0244123456" />
          </FormField>
          <FormField label="Subscriber Name (auto-creates walk-in if new)">
            <TextInput v-model="fullName" placeholder="Ama Mensah" />
          </FormField>
          <FormField label="Internet Bundle to Activate" required>
            <SelectInput
              v-model="selectedPlanId"
              :options="plans.map(p => ({ value: p.id, label: `${p.name} — ${formatCurrency(p.price)}` }))"
            />
          </FormField>
          <BaseButton block icon="bolt" :loading="busy" @click="activateBundle">
            Activate Bundle Immediately
          </BaseButton>
        </div>
      </BaseCard>
    </div>
  </div>
</template>
