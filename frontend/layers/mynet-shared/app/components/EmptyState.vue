<script setup lang="ts">
withDefaults(defineProps<{
  icon?: string
  title: string
  message?: string
}>(), {
  icon: 'information-circle',
})
</script>

<template>
  <div class="px-6 py-12 text-center">
    <span class="mb-4 inline-flex h-14 w-14 items-center justify-center rounded-full bg-canvas text-ink-faint">
      <AppIcon :name="icon" :size="26" />
    </span>
    <h3 class="text-sm font-semibold text-ink">{{ title }}</h3>
    <p v-if="message" class="mx-auto mt-1 max-w-sm text-xs text-ink-faint">{{ message }}</p>
    <div v-if="$slots.default" class="mt-4 flex justify-center">
      <slot />
    </div>
  </div>
</template>
