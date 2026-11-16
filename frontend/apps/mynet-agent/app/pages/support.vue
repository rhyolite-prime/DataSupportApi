<script setup lang="ts">
useHead({ title: 'NOC Field Support & Profile' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const data = ref<any>(null)
const showTicket = ref(false)
const form = reactive({
  zoneId: '',
  category: 'CONNECTIVITY',
  priority: 'HIGH',
  subscriberMsisdn: '',
  subject: '',
  description: '',
})

async function load() {
  data.value = await api.get('/agent/tickets')
  if (data.value?.agent && !form.zoneId) {
    form.zoneId = data.value.agent.territoryZoneId
  }
}

await load()

async function submitTicket() {
  try {
    await api.post('/agent/tickets', { ...form })
    toast.success('Escalated to MyNet NOC', 'Field engineering & support desk notified.')
    showTicket.value = false
    Object.assign(form, { subscriberMsisdn: '', subject: '', description: '' })
    await load()
  } catch (e: any) {
    toast.error('Could not submit ticket', e.message)
  }
}
</script>

<template>
  <div v-if="data" class="space-y-6">
    <PageHeader
      title="Territory NOC Escalation & Reseller KYC Profile"
      subtitle="Report WiFi sector outages, voucher redemption issues, or customer CPE faults directly to MyNet Network Operations."
      icon="lifebuoy"
    >
      <template #actions>
        <BaseButton size="sm" icon="plus" @click="showTicket = true">
          + Escalate field issue to NOC
        </BaseButton>
      </template>
    </PageHeader>

    <div class="grid gap-6 lg:grid-cols-12">
      <BaseCard class="lg:col-span-4" title="Verified Reseller Profile" icon="shield-check">
        <div class="space-y-3 text-xs">
          <div class="flex justify-between border-b border-line pb-2">
            <span class="text-ink-faint">Reseller Code</span>
            <code class="font-mono font-bold text-ink">{{ data.agent.agentCode }}</code>
          </div>
          <div class="flex justify-between border-b border-line pb-2">
            <span class="text-ink-faint">Business Name</span>
            <strong class="text-ink">{{ data.agent.name }}</strong>
          </div>
          <div class="flex justify-between border-b border-line pb-2">
            <span class="text-ink-faint">Commission Tier</span>
            <Badge tone="purple" size="sm">{{ data.agent.tier }} ({{ Math.round(data.agent.commissionRate * 100) }}%)</Badge>
          </div>
          <div class="flex justify-between border-b border-line pb-2">
            <span class="text-ink-faint">KYC Status</span>
            <Badge tone="green" size="sm" dot>{{ data.agent.kycStatus }}</Badge>
          </div>
          <div class="flex justify-between">
            <span class="text-ink-faint">NOC Hotline</span>
            <strong class="text-ink tnum">+233 30 274 8810</strong>
          </div>
        </div>
      </BaseCard>

      <BaseCard class="lg:col-span-8" title="Territory & Agent Support Tickets" :pad="false">
        <DataTable
          :columns="[
            { key: 'ticketNumber', label: 'Ticket' },
            { key: 'subject', label: 'Issue' },
            { key: 'priority', label: 'Priority' },
            { key: 'status', label: 'Status' },
            { key: 'createdAt', label: 'Opened' },
          ]"
          :rows="data.tickets"
        >
          <template #cell="{ col, row }">
            <template v-if="col.key === 'ticketNumber'">
              <code class="font-mono text-xs font-bold text-ink">{{ row.ticketNumber }}</code>
            </template>
            <template v-else-if="col.key === 'subject'">
              <p class="font-semibold text-ink">{{ row.subject }}</p>
              <p class="text-[11px] text-ink-faint">{{ row.zoneName }} · {{ row.category }}</p>
            </template>
            <template v-else-if="col.key === 'priority'">
              <Badge :tone="statusMeta(row.priority).tone" size="sm">{{ row.priority }}</Badge>
            </template>
            <template v-else-if="col.key === 'status'">
              <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
            </template>
            <template v-else-if="col.key === 'createdAt'">
              <span class="text-xs text-ink-faint">{{ formatRelativeTime(row.createdAt) }}</span>
            </template>
            <template v-else>{{ row[col.key] }}</template>
          </template>
        </DataTable>
      </BaseCard>
    </div>

    <ModalShell :open="showTicket" title="Escalate Field or Customer Issue to MyNet NOC" @close="showTicket = false">
      <div class="space-y-4">
        <div class="grid grid-cols-2 gap-3">
          <FormField label="Affected WiFi Zone">
            <SelectInput v-model="form.zoneId" :options="data.zones.map((z: any) => ({ value: z.id, label: z.name }))" />
          </FormField>
          <FormField label="Priority">
            <SelectInput
              v-model="form.priority"
              :options="[
                { value: 'MEDIUM', label: 'Medium' },
                { value: 'HIGH', label: 'High' },
                { value: 'URGENT', label: 'Urgent (Sector Down)' },
              ]"
            />
          </FormField>
        </div>
        <FormField label="Affected Subscriber Phone (Optional)">
          <TextInput v-model="form.subscriberMsisdn" placeholder="0244123456" />
        </FormField>
        <FormField label="Short Summary" required>
          <TextInput v-model="form.subject" placeholder="Weak WiFi signal near Kaneshie North Lane" />
        </FormField>
        <FormField label="Details for Field Engineer">
          <TextareaInput v-model="form.description" placeholder="Describe symptoms, voucher serial, or pole location…" />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showTicket = false">Cancel</BaseButton>
        <BaseButton size="sm" @click="submitTicket">Submit to NOC</BaseButton>
      </template>
    </ModalShell>
  </div>
</template>
