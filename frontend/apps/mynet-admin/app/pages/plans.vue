<script setup lang="ts">
import type { MyNetDataPlan } from '@mynet-shared/types/models'

useHead({ title: 'Internet Data Plans' })

const api = useMyNetApi()
const toast = useToast()

const plans = ref<MyNetDataPlan[]>([])
const showModal = ref(false)
const editingId = ref<string | null>(null)
const simPlan = ref<MyNetDataPlan | null>(null)

const form = reactive({
  planCode: '',
  name: '',
  planType: 'VOLUME',
  durationMinutes: 1440,
  dataCapMb: 5120,
  downKbps: 15000,
  upKbps: 8000,
  deviceLimit: 1,
  price: 10,
  wholesalePrice: 8.5,
  validityDays: 1,
  isPublic: true,
})

async function load() {
  plans.value = await api.get<MyNetDataPlan[]>('/admin/plans')
}

await load()

function openCreate() {
  editingId.value = null
  Object.assign(form, {
    planCode: '',
    name: '',
    planType: 'VOLUME',
    durationMinutes: 1440,
    dataCapMb: 5120,
    downKbps: 15000,
    upKbps: 8000,
    deviceLimit: 1,
    price: 10,
    wholesalePrice: 8.5,
    validityDays: 3,
    isPublic: true,
  })
  showModal.value = true
}

function openEdit(p: MyNetDataPlan) {
  editingId.value = p.id
  Object.assign(form, {
    planCode: p.planCode,
    name: p.name,
    planType: p.planType,
    durationMinutes: p.durationMinutes,
    dataCapMb: p.dataCapMb,
    downKbps: p.downKbps,
    upKbps: p.upKbps,
    deviceLimit: p.deviceLimit,
    price: p.price,
    wholesalePrice: p.wholesalePrice,
    validityDays: p.validityDays,
    isPublic: p.isPublic,
  })
  showModal.value = true
}

async function savePlan() {
  try {
    if (editingId.value) {
      await api.put(`/admin/plans/${editingId.value}`, { ...form })
      toast.success('Plan updated', `${form.planCode} synced to RADIUS rate profiles.`)
    } else {
      await api.post('/admin/plans', { ...form })
      toast.success('Plan created', `${form.name} published to MyNet portal & agent POS.`)
    }
    showModal.value = false
    await load()
  } catch (e: any) {
    toast.error('Could not save plan', e.message)
  }
}

const simTaxSplit = computed(() => {
  if (!simPlan.value) return null
  const gross = simPlan.value.price
  const net = Number((gross / 1.2).toFixed(2))
  return {
    gross,
    net,
    vat: Number((net * 0.15).toFixed(2)),
    nhil: Number((net * 0.025).toFixed(2)),
    getFund: Number((net * 0.025).toFixed(2)),
    agentMargin: simPlan.value.agentCommission,
    rateAttr: `${simPlan.value.downKbps}k/${simPlan.value.upKbps}k`,
  }
})
</script>

<template>
  <div class="space-y-6">
    <PageHeader
      title="Internet Plans & Tariff Catalogue"
      subtitle="Hotspot walk-in passes and PPPoE home/business plans, MikroTik rate attributes, and reseller margins."
      icon="bolt"
    >
      <template #actions>
        <BaseButton size="sm" icon="plus" @click="openCreate">
          + New internet plan
        </BaseButton>
      </template>
    </PageHeader>

    <div class="grid gap-5 md:grid-cols-2 xl:grid-cols-3">
      <BaseCard v-for="p in plans" :key="p.id">
        <div class="flex items-start justify-between gap-2">
          <div>
            <div class="flex items-center gap-2">
              <code class="rounded-md bg-primary-soft px-2 py-0.5 font-mono text-xs font-bold text-primary">{{ p.planCode }}</code>
              <Badge :tone="p.planType === 'UNLIMITED' ? 'purple' : p.planType === 'HYBRID' ? 'green' : 'blue'" size="sm">
                {{ p.planType }}
              </Badge>
            </div>
            <h3 class="mt-2 text-base font-bold text-ink">{{ p.name }}</h3>
          </div>
          <div class="text-right tnum">
            <p class="text-xl font-extrabold text-ink">{{ formatCurrency(p.price) }}</p>
            <p class="text-[11px] text-emerald-600 dark:text-emerald-400">Agent +{{ formatCurrency(p.agentCommission) }}</p>
          </div>
        </div>

        <div class="mt-4 grid grid-cols-2 gap-2.5 rounded-xl border border-line bg-canvas/50 p-3 text-xs tnum">
          <div>
            <span class="text-ink-faint">Allowance</span>
            <p class="mt-0.5 font-semibold text-ink">{{ formatDataVolume(p.dataCapMb) }}</p>
          </div>
          <div>
            <span class="text-ink-faint">Duration</span>
            <p class="mt-0.5 font-semibold text-ink">{{ formatDurationMinutes(p.durationMinutes) }}</p>
          </div>
          <div>
            <span class="text-ink-faint">Speed Profile</span>
            <p class="mt-0.5 font-semibold text-ink">{{ formatSpeedKbps(p.downKbps) }} / {{ formatSpeedKbps(p.upKbps) }}</p>
          </div>
          <div>
            <span class="text-ink-faint">Device Limit</span>
            <p class="mt-0.5 font-semibold text-ink">{{ p.deviceLimit }} device{{ p.deviceLimit > 1 ? 's' : '' }}</p>
          </div>
        </div>

        <div class="mt-4 flex items-center justify-between border-t border-line pt-3 text-xs">
          <span class="text-ink-faint tnum">
            <strong class="text-ink">{{ p.activeSubscriptions }}</strong> live · <strong class="text-ink">{{ p.vouchersAvailable }}</strong> vouchers
          </span>
          <div class="flex items-center gap-1.5">
            <BaseButton size="sm" variant="ghost" @click="simPlan = p">Simulate</BaseButton>
            <BaseButton size="sm" variant="secondary" @click="openEdit(p)">Edit</BaseButton>
          </div>
        </div>
      </BaseCard>
    </div>

    <!-- Plan Create/Edit Modal -->
    <ModalShell :open="showModal" :title="editingId ? 'Edit Internet Plan' : 'Create Internet Plan'" width="max-w-xl" @close="showModal = false">
      <div class="grid gap-4 sm:grid-cols-2">
        <FormField label="Plan Code" required>
          <TextInput v-model="form.planCode" :disabled="Boolean(editingId)" placeholder="WEEKEND-MAX" />
        </FormField>
        <FormField label="Commercial Name" required>
          <TextInput v-model="form.name" placeholder="MyNet Weekend Blast 25GB" />
        </FormField>
        <FormField label="Plan Type">
          <SelectInput
            v-model="form.planType"
            :options="[
              { value: 'TIME', label: 'Time Pass' },
              { value: 'VOLUME', label: 'Volume Cap' },
              { value: 'HYBRID', label: 'Hybrid + Fair Use' },
              { value: 'UNLIMITED', label: 'Unlimited Monthly' },
            ]"
          />
        </FormField>
        <FormField label="Data Cap (MB, 0 = Unlimited)">
          <TextInput v-model="form.dataCapMb" type="number" min="0" />
        </FormField>
        <FormField label="Retail Price (GH₵)" required>
          <TextInput v-model="form.price" type="number" step="0.5" min="1" />
        </FormField>
        <FormField label="Agent Wholesale Price (GH₵)" required>
          <TextInput v-model="form.wholesalePrice" type="number" step="0.25" min="0.5" />
        </FormField>
        <FormField label="Download Speed (kbps)">
          <TextInput v-model="form.downKbps" type="number" step="1000" />
        </FormField>
        <FormField label="Upload Speed (kbps)">
          <TextInput v-model="form.upKbps" type="number" step="1000" />
        </FormField>
        <FormField label="Simultaneous Devices">
          <TextInput v-model="form.deviceLimit" type="number" min="1" max="10" />
        </FormField>
        <FormField label="Validity (Days)">
          <TextInput v-model="form.validityDays" type="number" min="1" />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showModal = false">Cancel</BaseButton>
        <BaseButton size="sm" @click="savePlan">Save Plan</BaseButton>
      </template>
    </ModalShell>

    <!-- Tax & RADIUS Simulation Modal -->
    <ModalShell
      :open="Boolean(simPlan)"
      :title="`Tariff & RADIUS Simulation · ${simPlan?.planCode || ''}`"
      subtitle="Matches MagmaPlanService::simulatePlan & Ghana statutory tax split"
      @close="simPlan = null"
    >
      <div v-if="simPlan && simTaxSplit" class="space-y-3 text-xs">
        <div class="rounded-xl border border-line bg-canvas/60 p-4 space-y-2 tnum">
          <div class="flex justify-between"><span>Retail Price (Tax-Inclusive)</span><strong class="text-ink">{{ formatCurrency(simTaxSplit.gross) }}</strong></div>
          <div class="flex justify-between"><span>Net Revenue (Ex-Tax)</span><span>{{ formatCurrency(simTaxSplit.net) }}</span></div>
          <div class="flex justify-between text-ink-faint"><span>Ghana VAT (15%)</span><span>{{ formatCurrency(simTaxSplit.vat) }}</span></div>
          <div class="flex justify-between text-ink-faint"><span>NHIL (2.5%) + GETFund (2.5%)</span><span>{{ formatCurrency(simTaxSplit.nhil + simTaxSplit.getFund) }}</span></div>
          <div class="flex justify-between border-t border-line pt-2 text-emerald-600 font-semibold">
            <span>Default Agent Commission</span><span>{{ formatCurrency(simTaxSplit.agentMargin) }}</span>
          </div>
        </div>
        <div class="rounded-xl border border-line bg-canvas/60 p-4 font-mono text-[11px] space-y-1">
          <p class="font-sans text-xs font-semibold text-ink mb-1">FreeRADIUS radreply Projection:</p>
          <p>Mikrotik-Rate-Limit = "{{ simTaxSplit.rateAttr }}"</p>
          <p>Simultaneous-Use := "{{ simPlan.deviceLimit }}"</p>
          <p>Acct-Interim-Interval := "60"</p>
        </div>
      </div>
      <template #footer>
        <BaseButton size="sm" @click="simPlan = null">Done</BaseButton>
      </template>
    </ModalShell>
  </div>
</template>
