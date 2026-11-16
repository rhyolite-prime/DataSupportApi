<script setup lang="ts">
defineProps<{
  title: string
  subtitle?: string
  icon?: string
}>()
</script>

<template>
  <div class="mb-6 flex flex-wrap items-start justify-between gap-4">
    <div class="flex items-center gap-3.5">
      <span
        v-if="icon"
        class="flex h-11 w-11 shrink-0 items-center justify-center rounded-xl bg-primary text-on-primary shadow-sm"
      >
        <AppIcon :name="icon" :size="22" />
      </span>
      <div>
        <h1 class="text-xl font-bold tracking-tight text-ink">{{ title }}</h1>
        <p v-if="subtitle" class="mt-0.5 text-sm text-ink-faint">{{ subtitle }}</p>
      </div>
    </div>
    <div v-if="$slots.default || $slots.actions" class="flex flex-wrap items-center gap-2">
      <slot name="actions" />
      <slot />
    </div>
  </div>
</template>
