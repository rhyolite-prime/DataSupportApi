<script setup lang="ts">
defineProps<{
  modelValue: string | number | null | undefined
  options: { value: string | number; label: string }[]
  placeholder?: string
  disabled?: boolean
  allowEmpty?: boolean
}>()
defineEmits<{ 'update:modelValue': [v: any] }>()
</script>

<template>
  <select
    :value="modelValue ?? ''"
    :disabled="disabled"
    class="w-full rounded-xl border border-line bg-surface px-3.5 py-2.5 text-sm text-ink transition focus:border-primary focus:outline-none focus:ring-2 focus:ring-primary/25 disabled:cursor-not-allowed disabled:bg-canvas"
    @change="$emit('update:modelValue', ($event.target as HTMLSelectElement).value)"
  >
    <option v-if="placeholder" value="" :disabled="!allowEmpty">{{ placeholder }}</option>
    <option v-for="o in options" :key="o.value" :value="o.value">{{ o.label }}</option>
  </select>
</template>
