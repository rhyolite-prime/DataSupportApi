<script setup lang="ts">
const { brand } = useBrand()
const props = withDefaults(defineProps<{
  dark?: boolean
  productName?: string
  compact?: boolean
}>(), {
  dark: false,
  productName: undefined,
  compact: false,
})
</script>

<template>
  <div class="flex min-w-0 items-center gap-3">
    <span
      class="relative flex h-10 w-10 shrink-0 items-center justify-center rounded-xl text-sidebar shadow-md"
      style="background: linear-gradient(135deg, var(--brand-accent), color-mix(in srgb, var(--brand-accent) 70%, white));"
    >
      <svg width="21" height="21" viewBox="0 0 24 24" fill="none" stroke="#0B132B" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
        <path d="M5 12.55a11 11 0 0 1 14.08 0" />
        <path d="M1.42 9a16 16 0 0 1 21.16 0" />
        <path d="M8.53 16.11a6 6 0 0 1 6.95 0" />
        <circle cx="12" cy="19.5" r="1.2" fill="#0B132B" />
      </svg>
    </span>
    <div v-if="!compact" class="min-w-0">
      <div class="flex items-center gap-1.5">
        <p class="truncate text-[16px] font-extrabold leading-tight tracking-tight" :class="dark ? 'text-white' : 'text-ink'">
          MyNet
        </p>
        <span
          class="rounded-md px-1.5 py-0.5 text-[9px] font-bold uppercase tracking-wider"
          :class="dark ? 'bg-white/12 text-white/90' : 'bg-primary-soft text-primary'"
        >
          {{ brand.portalKind === 'agent' ? 'Agent' : 'Admin' }}
        </span>
      </div>
      <p class="mt-0.5 truncate text-[10px] font-semibold uppercase tracking-[0.14em]" :class="dark ? 'text-white/50' : 'text-ink-faint'">
        {{ props.productName || brand.productName }}
      </p>
    </div>
  </div>
</template>
