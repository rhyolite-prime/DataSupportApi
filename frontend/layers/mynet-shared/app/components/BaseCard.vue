<script setup lang="ts">
withDefaults(defineProps<{
  title?: string
  subtitle?: string
  icon?: string
  pad?: boolean
}>(), {
  pad: true,
})
</script>

<template>
  <section class="rounded-2xl border border-line bg-surface shadow-[0_1px_2px_rgba(15,23,42,0.04)]">
    <header
      v-if="title || $slots.actions || $slots.header"
      class="flex flex-wrap items-start justify-between gap-3 border-b border-line px-5 py-4"
    >
      <div class="flex min-w-0 items-center gap-3">
        <span v-if="icon" class="flex h-9 w-9 shrink-0 items-center justify-center rounded-xl bg-primary-soft text-primary">
          <AppIcon :name="icon" :size="18" />
        </span>
        <div class="min-w-0">
          <h3 class="truncate text-sm font-semibold text-ink">{{ title }}</h3>
          <p v-if="subtitle" class="mt-0.5 text-xs text-ink-faint">{{ subtitle }}</p>
        </div>
      </div>
      <div v-if="$slots.actions" class="flex shrink-0 flex-wrap items-center gap-2">
        <slot name="actions" />
      </div>
      <slot name="header" />
    </header>
    <div :class="pad ? 'p-5' : ''">
      <slot />
    </div>
    <footer v-if="$slots.footer" class="rounded-b-2xl border-t border-line bg-canvas/50 px-5 py-3">
      <slot name="footer" />
    </footer>
  </section>
</template>
