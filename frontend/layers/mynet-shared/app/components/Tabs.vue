<script setup lang="ts">
export interface TabItem {
  key: string
  label: string
  icon?: string
  count?: number
}

defineProps<{
  modelValue: string
  tabs: TabItem[]
}>()
const emit = defineEmits<{ 'update:modelValue': [v: string] }>()
</script>

<template>
  <div class="flex items-center gap-1 overflow-x-auto border-b border-line">
    <button
      v-for="t in tabs"
      :key="t.key"
      type="button"
      class="relative cursor-pointer whitespace-nowrap px-4 py-2.5 text-xs font-semibold transition-colors"
      :class="modelValue === t.key ? 'text-primary' : 'text-ink-faint hover:text-ink-soft'"
      @click="emit('update:modelValue', t.key)"
    >
      <span class="inline-flex items-center gap-2">
        <AppIcon v-if="t.icon" :name="t.icon" :size="15" />
        {{ t.label }}
        <span
          v-if="t.count !== undefined"
          class="rounded-full px-1.5 py-0.5 text-[10px] font-bold tnum"
          :class="modelValue === t.key ? 'bg-primary-soft text-primary' : 'bg-canvas text-ink-faint'"
        >
          {{ t.count }}
        </span>
      </span>
      <span v-if="modelValue === t.key" class="absolute inset-x-3 -bottom-px h-0.5 rounded-full bg-primary" />
    </button>
  </div>
</template>
