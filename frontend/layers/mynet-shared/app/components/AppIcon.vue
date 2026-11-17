<script setup lang="ts">
const props = withDefaults(defineProps<{ name: string; size?: number | string }>(), { size: 20 })
const paths = computed(() => ICON_PATHS[props.name] ?? ICON_PATHS['information-circle'] ?? [])
</script>

<template>
  <svg
    :width="size"
    :height="size"
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="1.75"
    stroke-linecap="round"
    stroke-linejoin="round"
    aria-hidden="true"
    class="shrink-0"
  >
    <path v-for="(d, i) in paths" :key="i" :d="d" />
  </svg>
</template>
