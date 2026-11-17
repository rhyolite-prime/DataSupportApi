<script setup lang="ts">
defineProps<{
  modelValue: string | number | null | undefined
  type?: string
  placeholder?: string
  disabled?: boolean
  required?: boolean
  autocomplete?: string
  inputmode?: 'text' | 'numeric' | 'tel' | 'decimal' | 'email' | 'search'
  maxlength?: number | string
  min?: number | string
  max?: number | string
  step?: number | string
}>()
defineEmits<{ 'update:modelValue': [v: any] }>()
</script>

<template>
  <input
    :type="type ?? 'text'"
    :value="modelValue ?? ''"
    :placeholder="placeholder"
    :disabled="disabled"
    :required="required"
    :autocomplete="autocomplete"
    :inputmode="inputmode"
    :maxlength="maxlength"
    :min="min"
    :max="max"
    :step="step"
    class="w-full rounded-xl border border-line bg-surface px-3.5 py-2.5 text-sm text-ink placeholder:text-ink-faint transition focus:border-primary focus:outline-none focus:ring-2 focus:ring-primary/25 disabled:cursor-not-allowed disabled:bg-canvas"
    @input="$emit('update:modelValue', ($event.target as HTMLInputElement).value)"
  >
</template>
