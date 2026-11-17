<script setup lang="ts">
definePageMeta({ layout: 'auth' })
useHead({ title: 'Sign in' })

const { loginAdmin } = usePortalSession('admin')
const toast = useToast()

const mode = ref<'pin' | 'apikey'>('pin')
const busy = ref(false)
const form = reactive({
  msisdn: '0244000001',
  pin: '2468',
  clientId: 'mynet_live_accra_01',
  clientSecret: 'demo-secret-key-2026',
})

const demoProfiles = [
  { label: 'Nana Kwame (Admin)', msisdn: '0244000001', role: 'ADMIN' },
  { label: 'Araba (Network Ops)', msisdn: '0244000002', role: 'OPS' },
  { label: 'Yaw (Field Engineer)', msisdn: '0244777888', role: 'TECHNICIAN' },
  { label: 'Kojo (Revenue & Tax)', msisdn: '0244000005', role: 'FINANCE' },
]

function selectProfile(msisdn: string) {
  mode.value = 'pin'
  form.msisdn = msisdn
  form.pin = '2468'
}

async function submit() {
  if (busy.value) return
  busy.value = true
  try {
    await loginAdmin({
      mode: mode.value,
      msisdn: form.msisdn,
      pin: form.pin,
      clientId: form.clientId,
      clientSecret: form.clientSecret,
    })
    toast.success('Signed in', 'Welcome to MyNet Network Control Plane.')
    await navigateTo('/')
  } catch (e: any) {
    toast.error('Sign-in failed', e.message)
  } finally {
    busy.value = false
  }
}
</script>

<template>
  <PortalAuthShell
    :headline="'Every zone connected.\nEvery subscriber in control.'"
    description="Unified ISP & community WiFi operations for MyNet"
    :highlights="[
      { icon: 'server-stack', title: 'WireGuard & RouterOS v7 Control', detail: 'Remote NAS enrolment, walled-garden sync, and UDP/3799 CoA disconnects.' },
      { icon: 'ticket', title: 'HMAC Voucher & Agent Distribution', detail: 'Batch minting, wholesale float wallets, and instant MoMo settlements.' },
      { icon: 'chart-bar-square', title: 'Ghana Statutory Tax Compliance', detail: 'Automated VAT, NHIL & GETFund splits alongside per-site backhaul P&L.' },
    ]"
  >
    <div class="rounded-2xl border border-line bg-surface p-6 shadow-sm sm:p-8">
      <div class="flex items-center justify-between gap-2">
        <div>
          <h1 class="text-2xl font-bold text-ink">Admin Sign In</h1>
          <p class="mt-1 text-xs text-ink-faint">Sign in with Staff PIN or Tenant API Credentials</p>
        </div>
        <Badge tone="blue" dot>Demo Mode</Badge>
      </div>

      <div class="mt-5 grid grid-cols-2 gap-1.5 rounded-xl bg-canvas p-1">
        <button
          type="button"
          class="cursor-pointer rounded-lg py-2 text-xs font-semibold transition"
          :class="mode === 'pin' ? 'bg-surface text-ink shadow-sm' : 'text-ink-faint hover:text-ink'"
          @click="mode = 'pin'"
        >
          Staff Phone + PIN
        </button>
        <button
          type="button"
          class="cursor-pointer rounded-lg py-2 text-xs font-semibold transition"
          :class="mode === 'apikey' ? 'bg-surface text-ink shadow-sm' : 'text-ink-faint hover:text-ink'"
          @click="mode = 'apikey'"
        >
          ClientId / Secret
        </button>
      </div>

      <form class="mt-5 space-y-4" @submit.prevent="submit">
        <template v-if="mode === 'pin'">
          <FormField label="Staff Phone or Email" required>
            <TextInput v-model="form.msisdn" placeholder="0244000001" autocomplete="username" required />
          </FormField>
          <FormField label="Operator 4-Digit PIN" hint="Seeded demo PIN for all staff accounts: 2468" required>
            <TextInput v-model="form.pin" type="password" inputmode="numeric" maxlength="6" placeholder="••••" required />
          </FormField>
        </template>

        <template v-else>
          <FormField label="Tenant ClientId" required>
            <TextInput v-model="form.clientId" placeholder="mynet_live_accra_01" required />
          </FormField>
          <FormField label="Tenant ClientSecret" required>
            <TextInput v-model="form.clientSecret" type="password" placeholder="••••••••••••" required />
          </FormField>
        </template>

        <BaseButton type="submit" block icon="arrow-right" :loading="busy">
          Open MyNet Control Plane
        </BaseButton>
      </form>

      <div class="mt-6 border-t border-line pt-4">
        <p class="mb-2 text-[11px] font-bold uppercase tracking-wider text-ink-faint">
          Quick Demo Operator Switch
        </p>
        <div class="grid grid-cols-2 gap-2">
          <button
            v-for="p in demoProfiles"
            :key="p.msisdn"
            type="button"
            class="flex cursor-pointer flex-col rounded-xl border border-line bg-canvas/60 px-3 py-2 text-left transition hover:border-primary/50 hover:bg-primary-soft/40"
            @click="selectProfile(p.msisdn)"
          >
            <span class="truncate text-xs font-semibold text-ink">{{ p.label }}</span>
            <span class="text-[10px] font-medium text-ink-faint">{{ p.msisdn }} · {{ p.role }}</span>
          </button>
        </div>
      </div>
    </div>
  </PortalAuthShell>
</template>
