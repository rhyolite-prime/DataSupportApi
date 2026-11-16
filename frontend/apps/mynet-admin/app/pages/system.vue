<script setup lang="ts">
useHead({ title: 'Staff, API Keys & Audit' })

const api = useMyNetApi()
const toast = useToast()
const { statusMeta } = useStatusMeta()

const tab = ref('staff')
const data = ref<any>(null)
const showAddStaff = ref(false)
const staffForm = reactive({
  fullName: '',
  msisdn: '',
  email: '',
  role: 'OPS',
  pin: '2468',
})

async function load() {
  data.value = await api.get('/admin/system')
}

await load()

async function createStaff() {
  try {
    await api.post('/admin/staff', { ...staffForm })
    toast.success('Operator added', `${staffForm.fullName} provisioned with bcrypt PIN hash.`)
    showAddStaff.value = false
    Object.assign(staffForm, { fullName: '', msisdn: '', email: '' })
    await load()
  } catch (e: any) {
    toast.error('Could not add operator', e.message)
  }
}
</script>

<template>
  <div v-if="data" class="space-y-6">
    <PageHeader
      title="Staff Roles, API Integration & Audit Trail"
      subtitle="Role-based operator directory, DataSupportApi connection profile, and immutable magma_audit_log records."
      icon="shield-check"
    >
      <template #actions>
        <BaseButton size="sm" icon="plus" @click="showAddStaff = true">
          + Add staff operator
        </BaseButton>
      </template>
    </PageHeader>

    <Tabs
      v-model="tab"
      :tabs="[
        { key: 'staff', label: 'Staff Operators', icon: 'users', count: data.staff.length },
        { key: 'api', label: 'DataSupportApi Integration', icon: 'key' },
        { key: 'audit', label: 'Audit Trail (magma_audit_log)', icon: 'clipboard-document-check', count: data.auditLog.length },
      ]"
    />

    <BaseCard v-if="tab === 'staff'" :pad="false">
      <DataTable
        :columns="[
          { key: 'fullName', label: 'Operator' },
          { key: 'role', label: 'Role' },
          { key: 'msisdn', label: 'Phone' },
          { key: 'zoneNames', label: 'Permitted Zones' },
          { key: 'lastLoginAt', label: 'Last Active' },
          { key: 'status', label: 'Status' },
        ]"
        :rows="data.staff"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'fullName'">
            <div class="font-semibold text-ink">{{ row.fullName }}</div>
            <div class="text-[11px] text-ink-faint">{{ row.email }}</div>
          </template>
          <template v-else-if="col.key === 'role'">
            <Badge tone="purple" size="sm">{{ row.role }}</Badge>
          </template>
          <template v-else-if="col.key === 'zoneNames'">
            <span class="text-xs">{{ row.zoneNames.join(', ') }}</span>
          </template>
          <template v-else-if="col.key === 'lastLoginAt'">
            <span class="text-xs text-ink-faint">{{ formatRelativeTime(row.lastLoginAt) }}</span>
          </template>
          <template v-else-if="col.key === 'status'">
            <Badge :tone="statusMeta(row.status).tone" size="sm" dot>{{ row.status }}</Badge>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <BaseCard
      v-else-if="tab === 'api'"
      title="Backend Control-Plane Cutover Configuration"
      subtitle="When UI sign-off is complete, point Nitro to the live Drogon DataSupportApi service without editing any Vue pages."
      icon="key"
    >
      <div class="grid gap-4 sm:grid-cols-2 text-xs">
        <div class="rounded-xl border border-line bg-canvas/50 p-4">
          <span class="text-ink-faint">Commercial Brand</span>
          <p class="mt-1 text-base font-bold text-ink">MyNet Broadband</p>
          <span class="text-[11px] text-ink-faint">Engineering Control Plane: MagmaNetService</span>
        </div>
        <div class="rounded-xl border border-line bg-canvas/50 p-4">
          <span class="text-ink-faint">Tenant Business UUID</span>
          <p class="mt-1 font-mono font-bold text-ink">{{ data.business.id }}</p>
          <span class="text-[11px] text-ink-faint">ClientId: {{ data.business.clientId }}</span>
        </div>
      </div>

      <div class="mt-4 rounded-xl border border-line bg-slate-950 p-4 font-mono text-xs text-slate-200">
        <p class="text-slate-400"># Cut over from stateful Nitro mock engine to live C++ DataSupportApi (:5702)</p>
        <p class="mt-1">NUXT_MAGMA_API_BASE_URL=https://api.mynet.gh</p>
        <p>MAGMA_CLIENT_ID={{ data.business.clientId }}</p>
        <p>MAGMA_CLIENT_SECRET={{ data.business.clientSecretHint }}</p>
      </div>
    </BaseCard>

    <BaseCard v-else :pad="false">
      <DataTable
        :columns="[
          { key: 'action', label: 'Action' },
          { key: 'actorName', label: 'Actor' },
          { key: 'summary', label: 'Change Summary' },
          { key: 'ipAddress', label: 'Origin IP' },
          { key: 'createdAt', label: 'Time' },
        ]"
        :rows="data.auditLog"
      >
        <template #cell="{ col, row }">
          <template v-if="col.key === 'action'">
            <code class="font-mono text-xs font-bold text-primary">{{ row.action }}</code>
          </template>
          <template v-else-if="col.key === 'actorName'">
            <span class="font-medium text-ink">{{ row.actorName }}</span>
            <span class="ml-1 text-[10px] text-ink-faint">({{ row.actorType }})</span>
          </template>
          <template v-else-if="col.key === 'ipAddress'">
            <code class="font-mono text-xs">{{ row.ipAddress }}</code>
          </template>
          <template v-else-if="col.key === 'createdAt'">
            <span class="text-xs text-ink-faint">{{ formatRelativeTime(row.createdAt) }}</span>
          </template>
          <template v-else>{{ row[col.key] }}</template>
        </template>
      </DataTable>
    </BaseCard>

    <ModalShell :open="showAddStaff" title="Add MyNet Staff Operator" @close="showAddStaff = false">
      <div class="space-y-4">
        <FormField label="Full Name" required>
          <TextInput v-model="staffForm.fullName" placeholder="Kwabena Mensah" />
        </FormField>
        <div class="grid grid-cols-2 gap-3">
          <FormField label="Phone (MSISDN)" required>
            <TextInput v-model="staffForm.msisdn" placeholder="0244112233" />
          </FormField>
          <FormField label="Role">
            <SelectInput
              v-model="staffForm.role"
              :options="[
                { value: 'ADMIN', label: 'Admin' },
                { value: 'OPS', label: 'Network Ops' },
                { value: 'TECHNICIAN', label: 'Field Technician' },
                { value: 'SUPPORT', label: 'Support Desk' },
                { value: 'FINANCE', label: 'Finance & Tax' },
              ]"
            />
          </FormField>
        </div>
        <FormField label="Email Address">
          <TextInput v-model="staffForm.email" type="email" placeholder="kwabena@mynet.gh" />
        </FormField>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showAddStaff = false">Cancel</BaseButton>
        <BaseButton size="sm" @click="createStaff">Create Operator</BaseButton>
      </template>
    </ModalShell>
  </div>
</template>
