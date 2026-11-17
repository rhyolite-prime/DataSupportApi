<script setup lang="ts">
import type { MyNetStaffMember, MyNetTicket, MyNetWorkOrder, MyNetZone } from '@mynet-shared/types/models'

useHead({ title: 'Support Desk & Work Orders' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const tab = ref('tickets')
const tickets = ref<MyNetTicket[]>([])
const workOrders = ref<MyNetWorkOrder[]>([])
const staff = ref<MyNetStaffMember[]>([])
const zones = ref<MyNetZone[]>([])

const activeTicket = ref<MyNetTicket | null>(null)
const replyBody = ref('')
const showNewWorkOrder = ref(false)

const woForm = reactive({
  zoneId: '',
  orderType: 'INSTALL',
  subscriberName: '',
  serviceAddress: '',
  assignedTo: '',
  cost: 0,
})

async function load() {
  const [supRes, zoneRes] = await Promise.all([
    api.get<{ tickets: MyNetTicket[]; workOrders: MyNetWorkOrder[]; staff: MyNetStaffMember[] }>('/admin/support'),
    api.get<{ zones: MyNetZone[] }>('/admin/zones'),
  ])
  tickets.value = supRes.tickets
  workOrders.value = supRes.workOrders
  staff.value = supRes.staff
  zones.value = zoneRes.zones
  if (!woForm.zoneId && zoneRes.zones[0]) woForm.zoneId = zoneRes.zones[0].id
  if (!woForm.assignedTo && supRes.staff[2]) woForm.assignedTo = supRes.staff[2].id
  if (activeTicket.value) {
    activeTicket.value = supRes.tickets.find(t => t.id === activeTicket.value?.id) || null
  }
}

await load()

async function sendReply(resolve = false) {
  if (!activeTicket.value) return
  try {
    await api.post(`/admin/tickets/${activeTicket.value.id}/reply`, {
      message: replyBody.value,
      status: resolve ? 'RESOLVED' : 'IN_PROGRESS',
    })
    replyBody.value = ''
    toast.success(resolve ? 'Ticket resolved' : 'Reply posted')
    await load()
  } catch (e: any) {
    toast.error('Failed to update ticket', e.message)
  }
}

async function createWorkOrder() {
  try {
    await api.post('/admin/work-orders', { ...woForm })
    toast.success('Work order dispatched')
    showNewWorkOrder.value = false
    Object.assign(woForm, { subscriberName: '', serviceAddress: '', cost: 0 })
    await load()
  } catch (e: any) {
    toast.error('Could not schedule work order', e.message)
  }
}

async function markWorkOrder(wo: MyNetWorkOrder, status: MyNetWorkOrder['status']) {
  try {
    await api.post(`/admin/work-orders/${wo.id}/status`, { status })
    toast.success(`${wo.orderNumber} marked ${status}`)
    await load()
  } catch (e: any) {
    toast.error('Update failed', e.message)
  }
}
</script>

<template>
  <div class="space-y-6">
    <PageHeader
      title="Support Helpdesk & Field Engineering Work Orders"
      subtitle="Customer & reseller tickets, SLA timers, CPE home installations, and sector repair dispatches."
      icon="wrench-screwdriver"
    >
      <template #actions>
        <BaseButton size="sm" icon="plus" @click="showNewWorkOrder = true">
          + Dispatch field work order
        </BaseButton>
      </template>
    </PageHeader>

    <Tabs
      v-model="tab"
      :tabs="[
        { key: 'tickets', label: 'Support Ticket Queue', icon: 'chat-bubble-left-right', count: tickets.length },
        { key: 'workorders', label: 'Field Work Orders', icon: 'wrench-screwdriver', count: workOrders.length },
      ]"
    />

    <BaseCard v-if="tab === 'tickets'" :pad="false">
      <DataTable
        clickable
        :columns="[
          { key: 'ticketNumber', label: 'Ticket' },
          { key: 'subject', label: 'Subject & Reporter' },
          { key: 'zoneName', label: 'Zone' },
          { key: 'priority', label: 'Priority' },
          { key: 'assignedToName', label: 'Owner' },
          { key: 'status', label: 'Status' },
        ]"
        :rows="tickets"
        @row-click="(r) => activeTicket = r"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'ticketNumber'">
            <code class="font-mono text-xs font-bold text-ink">{{ row.ticketNumber }}</code>
          </template>
          <template v-else-if="col.key === 'subject'">
            <p class="font-semibold text-ink">{{ row.subject }}</p>
            <p class="text-[11px] text-ink-faint">
              {{ row.subscriberName || row.agentName || 'Walk-in' }} · {{ row.category }}
            </p>
          </template>
          <template v-else-if="col.key === 'priority'">
            <Badge :tone="statusMeta(row.priority).tone" size="sm">{{ row.priority }}</Badge>
          </template>
          <template v-else-if="col.key === 'status'">
            <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ statusMeta(row.status).label }}</Badge>
          </template>
          <template v-else>{{ row[col.key] || '—' }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <div v-else class="grid gap-5 lg:grid-cols-3">
      <BaseCard v-for="wo in workOrders" :key="wo.id">
        <div class="flex items-center justify-between gap-2">
          <div class="flex items-center gap-2">
            <code class="font-mono text-xs font-bold text-ink">{{ wo.orderNumber }}</code>
            <Badge tone="purple" size="sm">{{ wo.orderType }}</Badge>
          </div>
          <Badge :tone="statusMeta(wo.status).tone" size="sm" dot>{{ statusMeta(wo.status).label }}</Badge>
        </div>

        <h3 class="mt-2 font-bold text-ink">{{ wo.serviceAddress }}</h3>
        <p class="text-xs text-ink-faint">{{ wo.zoneName }} · Engineer: {{ wo.assignedToName }}</p>

        <ul class="mt-3 space-y-1.5 rounded-xl border border-line bg-canvas/50 p-3 text-xs text-ink-soft">
          <li v-for="item in wo.checklist" :key="item" class="flex items-center gap-2">
            <AppIcon name="check-circle" :size="14" class="text-primary" />
            <span>{{ item }}</span>
          </li>
        </ul>

        <div class="mt-4 flex items-center justify-between border-t border-line pt-3 text-xs">
          <span class="text-ink-faint">Materials: {{ formatCurrency(wo.cost) }}</span>
          <div v-if="wo.status !== 'DONE'" class="flex gap-1.5">
            <BaseButton v-if="wo.status === 'SCHEDULED'" size="sm" variant="secondary" @click="markWorkOrder(wo, 'EN_ROUTE')">
              En Route
            </BaseButton>
            <BaseButton size="sm" variant="success" @click="markWorkOrder(wo, 'DONE')">
              Complete
            </BaseButton>
          </div>
        </div>
      </BaseCard>
    </div>

    <!-- Ticket Thread Modal -->
    <ModalShell
      :open="Boolean(activeTicket)"
      :title="`${activeTicket?.ticketNumber || ''} · ${activeTicket?.subject || ''}`"
      :subtitle="`${activeTicket?.zoneName || ''} · Assigned to ${activeTicket?.assignedToName || 'Support'}`"
      width="max-w-xl"
      @close="activeTicket = null"
    >
      <div v-if="activeTicket" class="space-y-4">
        <div class="max-h-64 space-y-2.5 overflow-y-auto rounded-xl border border-line bg-canvas/50 p-3.5">
          <div
            v-for="m in activeTicket.messages"
            :key="m.id"
            class="rounded-xl border border-line bg-surface p-3 text-xs"
          >
            <div class="flex justify-between font-semibold text-ink">
              <span>{{ m.authorName }} ({{ m.authorType }})</span>
              <span class="text-[11px] font-normal text-ink-faint">{{ formatRelativeTime(m.createdAt) }}</span>
            </div>
            <p class="mt-1 text-ink-soft">{{ m.body }}</p>
          </div>
        </div>
        <FormField label="Staff Response">
          <TextareaInput v-model="replyBody" placeholder="Type update or resolution notes…" />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="sendReply(false)">Post Reply</BaseButton>
        <BaseButton variant="success" size="sm" @click="sendReply(true)">Reply & Resolve</BaseButton>
      </template>
    </ModalShell>

    <!-- New Work Order Modal -->
    <ModalShell :open="showNewWorkOrder" title="Dispatch Field Engineering Work Order" @close="showNewWorkOrder = false">
      <div class="space-y-4">
        <div class="grid grid-cols-2 gap-3">
          <FormField label="Zone">
            <SelectInput v-model="woForm.zoneId" :options="zones.map(z => ({ value: z.id, label: z.name }))" />
          </FormField>
          <FormField label="Order Type">
            <SelectInput
              v-model="woForm.orderType"
              :options="[
                { value: 'INSTALL', label: 'PPPoE CPE Installation' },
                { value: 'REPAIR', label: 'Sector / PoE Repair' },
                { value: 'SURVEY', label: 'RF Site Survey' },
                { value: 'RECOVERY', label: 'Equipment Recovery' },
              ]"
            />
          </FormField>
        </div>
        <FormField label="Site / Subscriber Address" required>
          <TextInput v-model="woForm.serviceAddress" placeholder="Kaneshie North Lane, Shop 19" />
        </FormField>
        <div class="grid grid-cols-2 gap-3">
          <FormField label="Assign Technician">
            <SelectInput v-model="woForm.assignedTo" :options="staff.map(s => ({ value: s.id, label: `${s.fullName} (${s.role})` }))" />
          </FormField>
          <FormField label="Estimated Materials Cost (GH₵)">
            <TextInput v-model="woForm.cost" type="number" min="0" />
          </FormField>
        </div>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showNewWorkOrder = false">Cancel</BaseButton>
        <BaseButton size="sm" @click="createWorkOrder">Schedule Dispatch</BaseButton>
      </template>
    </ModalShell>
  </div>
</template>
