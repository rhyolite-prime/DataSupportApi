<script setup lang="ts">
defineProps<{
  modelValue: string
  placeholder?: string
  rows?: number
  disabled?: boolean
}>()
defineEmits<{ 'update:modelValue': [v: string] }>()
</script>

<template>
  <textarea
    :value="modelValue"
    :placeholder="placeholder"
    :rows="rows ?? 3"
    :disabled="disabled"
    class="w-full resize-y rounded-xl border border-line bg-surface px-3.5 py-2.5 text-sm text-ink placeholder:text-ink-faint transition focus:border-primary focus:outline-none focus:ring-2 focus:ring-primary/25"
    @input="$emit('update:modelValue', ($event.target as HTMLTextAreaElement).value)"
  />
</template>
