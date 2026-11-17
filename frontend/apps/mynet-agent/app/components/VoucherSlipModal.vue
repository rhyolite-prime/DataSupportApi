<script setup lang="ts">
import QRCode from 'qrcode'
import type { MyNetVoucher } from '@mynet-shared/types/models'

const props = defineProps<{
  open: boolean
  voucher: MyNetVoucher | null
  redeemUrl?: string
}>()

const emit = defineEmits<{ close: [] }>()
const toast = useToast()
const qrDataUrl = ref('')

const effectiveRedeemUrl = computed(() => {
  if (!props.voucher) return ''
  return (
    props.redeemUrl ||
    `https://wifi.mynet.gh/?zone=${encodeURIComponent(props.voucher.zoneCode)}&voucher=${encodeURIComponent(props.voucher.code)}`
  )
})

watch(
  () => [props.open, props.voucher?.code],
  async ([isOpen]) => {
    if (!isOpen || !props.voucher || !import.meta.client) return
    try {
      qrDataUrl.value = await QRCode.toDataURL(effectiveRedeemUrl.value, {
        width: 176,
        margin: 1,
        color: { dark: '#13112E', light: '#FFFFFF' },
      })
    } catch {
      qrDataUrl.value = ''
    }
  },
  { immediate: true },
)

async function copyPin() {
  if (!props.voucher || !import.meta.client) return
  await navigator.clipboard.writeText(props.voucher.code)
  toast.success('PIN copied', `${props.voucher.code} copied to clipboard.`)
}

function printSlip() {
  if (!import.meta.client) return
  window.print()
}
</script>

<template>
  <ModalShell
    :open="open"
    title="MyNet Instant WiFi Voucher Receipt"
    subtitle="Scan QR on MyNet Hotspot or enter the 12-character PIN at wifi.mynet.gh"
    width="max-w-md"
    @close="emit('close')"
  >
    <div v-if="voucher" class="space-y-4">
      <!-- Printable Thermal Slip -->
      <div class="rounded-2xl border-2 border-dashed border-line bg-canvas/40 p-5 text-center">
        <div class="flex items-center justify-center gap-2">
          <BrandMark :size="28" />
          <div class="text-left">
            <p class="text-sm font-extrabold text-ink">MyNet Community WiFi</p>
            <p class="text-[10px] font-semibold uppercase tracking-wider text-ink-faint">
              {{ voucher.zoneName }} · Serial {{ voucher.serial }}
            </p>
          </div>
        </div>

        <div class="my-4 rounded-xl border border-line bg-surface px-4 py-3 shadow-xs">
          <p class="text-[10px] font-bold uppercase tracking-[0.14em] text-ink-faint">Voucher PIN Code</p>
          <p class="mt-1 font-mono text-xl font-extrabold tracking-wider text-primary select-all">
            {{ voucher.code }}
          </p>
        </div>

        <div v-if="qrDataUrl" class="mx-auto my-3 inline-flex rounded-xl border border-line bg-white p-2.5 shadow-xs">
          <img :src="qrDataUrl" alt="MyNet WiFi Redeem QR" class="h-36 w-36" />
        </div>

        <div class="grid grid-cols-2 gap-2 rounded-xl border border-line bg-surface p-3 text-left text-xs tnum">
          <div>
            <span class="text-ink-faint">Bundle</span>
            <p class="font-bold text-ink">{{ voucher.planName }}</p>
          </div>
          <div>
            <span class="text-ink-faint">Retail Paid</span>
            <p class="font-extrabold text-ink">{{ formatCurrency(voucher.price) }}</p>
          </div>
          <div>
            <span class="text-ink-faint">Allowance</span>
            <p class="font-semibold text-ink">{{ voucher.allowanceLabel }}</p>
          </div>
          <div>
            <span class="text-ink-faint">Agent Earned</span>
            <p class="font-bold text-emerald-600 dark:text-emerald-400">+{{ formatCurrency(voucher.commission) }}</p>
          </div>
        </div>

        <p v-if="voucher.customerMsisdn" class="mt-3 text-[11px] text-ink-soft">
          SMS copy dispatched to <strong class="tnum">{{ voucher.customerMsisdn }}</strong>
        </p>
        <p class="mt-1 text-[10px] text-ink-faint">
          Connect to SSID <strong>MyNet-FreeWiFi</strong> → tap Sign In → enter PIN
        </p>
      </div>
    </div>

    <template #footer>
      <BaseButton variant="secondary" size="sm" icon="clipboard-document-check" @click="copyPin">
        Copy PIN
      </BaseButton>
      <BaseButton variant="secondary" size="sm" icon="printer" @click="printSlip">
        Print Slip
      </BaseButton>
      <BaseButton size="sm" @click="emit('close')">
        Done
      </BaseButton>
    </template>
  </ModalShell>
</template>
