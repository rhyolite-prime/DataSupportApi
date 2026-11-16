<script setup lang="ts">
const { brand, initBrand } = useBrand()
initBrand()

withDefaults(defineProps<{
  headline: string
  description: string
  highlights?: { icon: string; title: string; detail: string }[]
}>(), {
  highlights: () => [],
})
</script>

<template>
  <div class="flex min-h-screen" style="background: var(--color-canvas)">
    <!-- Brand Hero Side (mirrors BusinessDesk auth split layout, with MyNet broadband branding) -->
    <div
      class="relative hidden w-[44%] flex-col justify-between overflow-hidden p-10 text-white lg:flex"
      style="background: linear-gradient(155deg, var(--brand-primary-dark) 0%, var(--brand-sidebar) 75%);"
    >
      <div class="pointer-events-none absolute -top-28 -right-20 h-96 w-96 rounded-full opacity-15" style="background: var(--brand-accent)" />
      <div class="pointer-events-none absolute -bottom-16 -left-20 h-80 w-80 rounded-full bg-white/5" />

      <div class="relative flex items-center justify-between">
        <BrandMark dark />
        <span class="rounded-full border border-white/15 bg-white/10 px-3 py-1 text-[11px] font-semibold text-white/85">
          Ghana Broadband Cloud
        </span>
      </div>

      <div class="relative space-y-6">
        <div>
          <h2 class="text-3xl font-extrabold leading-tight tracking-tight whitespace-pre-line">
            {{ headline }}
          </h2>
          <p class="mt-3 max-w-md text-sm leading-relaxed text-white/75">
            {{ description }}
          </p>
        </div>

        <div v-if="highlights.length" class="grid gap-3 pt-2">
          <div
            v-for="item in highlights"
            :key="item.title"
            class="flex items-start gap-3.5 rounded-2xl border border-white/10 bg-white/6 p-3.5 backdrop-blur-sm"
          >
            <span
              class="flex h-9 w-9 shrink-0 items-center justify-center rounded-xl text-slate-950"
              style="background: var(--brand-accent)"
            >
              <AppIcon :name="item.icon" :size="18" />
            </span>
            <div>
              <p class="text-xs font-bold text-white">{{ item.title }}</p>
              <p class="mt-0.5 text-xs text-white/65">{{ item.detail }}</p>
            </div>
          </div>
        </div>
      </div>

      <p class="relative text-[11px] text-white/45">
        {{ brand.institutionName }} · {{ brand.tagline }}
      </p>
    </div>

    <!-- Form Side -->
    <div class="flex flex-1 items-center justify-center p-6 sm:p-10">
      <div class="w-full max-w-md">
        <div class="mb-8 flex justify-center lg:hidden">
          <BrandMark />
        </div>
        <slot />
      </div>
    </div>
    <ToastHost />
  </div>
</template>
