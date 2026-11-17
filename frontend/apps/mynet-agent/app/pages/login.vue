<script setup lang="ts">
definePageMeta({ layout: 'auth' })
useHead({ title: 'Reseller Sign In' })

const { loginAgent } = usePortalSession('agent')
const toast = useToast()

const busy = ref(false)
const showOtpReset = ref(false)
const otpSent = ref(false)
const resetMsisdn = ref('0244555666')
const resetOtp = ref('842910')
const resetNewPin = ref('1234')

const form = reactive({
  agentIdentifier: 'MYN-AG-001',
  pin: '1234',
})

const demoAgents = [
  { label: 'Kwesi Mensah Ventures', code: 'MYN-AG-001', tier: 'PREMIER · 15%', zone: 'Kaneshie' },
  { label: 'Adwoa Serwaa Enterprise', code: 'MYN-AG-002', tier: 'SUPER · 18%', zone: 'Madina' },
  { label: 'Issahaku Telecom Kiosk', code: 'MYN-AG-003', tier: 'STANDARD · 12%', zone: 'Mamprobi' },
]

function pickAgent(code: string) {
  form.agentIdentifier = code
  form.pin = '1234'
}

async function submit() {
  if (busy.value) return
  busy.value = true
  try {
    await loginAgent({
      agentIdentifier: form.agentIdentifier,
      pin: form.pin,
    })
    toast.success('Signed in', 'Welcome to MyNet Reseller POS.')
    await navigateTo('/')
  } catch (e: any) {
    toast.error('Sign-in failed', e.message)
  } finally {
    busy.value = false
  }
}

function handleSendOtp() {
  otpSent.value = true
  toast.info('SMS OTP dispatched', `6-digit reset code sent to ${resetMsisdn.value}.`)
}

function completeReset() {
  form.pin = resetNewPin.value
  showOtpReset.value = false
  otpSent.value = false
  toast.success('Agent PIN updated', 'Sign in with your new 4-digit PIN.')
}
</script>

<template>
  <PortalAuthShell
    :headline="'Sell WiFi in seconds.\nEarn MoMo commission on every pass.'"
    description="Built for MyNet Broadband market kiosks, mobile money vendors, and campus resellers — mint instant WiFi PINs, print QR receipts, top up subscribers directly, and cash out commissions anytime."
    :highlights="[
      { icon: 'bolt', title: 'Instant PIN & QR Receipt POS', detail: 'Issue from discounted stock or mint on demand from your TurboLedger float.' },
      { icon: 'wifi', title: 'Field Offline Queue & Sync', detail: 'Keep selling during brief backhaul dips and sync queued sales in one tap.' },
      { icon: 'wallet', title: 'Up to 18% Instant Commission', detail: 'Real-time wallet statements and MTN, Telecel, or AirtelTigo MoMo settlements.' },
    ]"
  >
    <div class="rounded-2xl border border-line bg-surface p-6 shadow-sm sm:p-8">
      <div class="flex items-center justify-between gap-2">
        <div>
          <h1 class="text-2xl font-bold text-ink">Reseller POS Sign In</h1>
          <p class="mt-1 text-xs text-ink-faint">Enter your MyNet Agent Code or Phone + 4-digit PIN</p>
        </div>
        <Badge tone="amber" dot>POS Ready</Badge>
      </div>

      <form class="mt-5 space-y-4" @submit.prevent="submit">
        <FormField label="Agent Code or Registered MoMo Phone" required>
          <TextInput v-model="form.agentIdentifier" placeholder="MYN-AG-001 or 0244555666" required />
        </FormField>

        <FormField label="4-Digit POS PIN" hint="Demo PIN for all seeded resellers: 1234" required>
          <TextInput v-model="form.pin" type="password" inputmode="numeric" maxlength="6" placeholder="••••" required />
        </FormField>

        <div class="flex items-center justify-end">
          <button
            type="button"
            class="cursor-pointer text-xs font-semibold text-primary hover:underline"
            @click="showOtpReset = true"
          >
            Forgot PIN? Reset via SMS OTP
          </button>
        </div>

        <BaseButton type="submit" block icon="bolt" :loading="busy">
          Open Reseller POS
        </BaseButton>
      </form>

      <div class="mt-6 border-t border-line pt-4">
        <p class="mb-2 text-[11px] font-bold uppercase tracking-wider text-ink-faint">
          Quick Demo Reseller Switch
        </p>
        <div class="space-y-2">
          <button
            v-for="a in demoAgents"
            :key="a.code"
            type="button"
            class="flex w-full cursor-pointer items-center justify-between rounded-xl border border-line bg-canvas/60 px-3.5 py-2.5 text-left text-xs transition hover:border-primary/40 hover:bg-primary-soft/40"
            @click="pickAgent(a.code)"
          >
            <div>
              <p class="font-bold text-ink">{{ a.label }}</p>
              <p class="text-[11px] text-ink-faint">{{ a.code }} · {{ a.zone }}</p>
            </div>
            <Badge tone="purple" size="sm">{{ a.tier }}</Badge>
          </button>
        </div>
      </div>
    </div>

    <ModalShell :open="showOtpReset" title="Reset Reseller POS PIN via SMS OTP" @close="showOtpReset = false">
      <div class="space-y-4">
        <FormField label="Registered Agent Phone (MSISDN)" required>
          <TextInput v-model="resetMsisdn" placeholder="0244555666" />
        </FormField>
        <BaseButton v-if="!otpSent" size="sm" variant="secondary" @click="handleSendOtp">
          Send 6-Digit SMS OTP
        </BaseButton>
        <template v-else>
          <div class="grid grid-cols-2 gap-3">
            <FormField label="SMS OTP Code">
              <TextInput v-model="resetOtp" />
            </FormField>
            <FormField label="New 4-Digit PIN">
              <TextInput v-model="resetNewPin" type="password" maxlength="4" />
            </FormField>
          </div>
        </template>
      </div>
      <template #footer>
        <BaseButton variant="secondary" size="sm" @click="showOtpReset = false">Cancel</BaseButton>
        <BaseButton v-if="otpSent" size="sm" @click="completeReset">Verify & Save PIN</BaseButton>
      </template>
    </ModalShell>
  </PortalAuthShell>
</template>
