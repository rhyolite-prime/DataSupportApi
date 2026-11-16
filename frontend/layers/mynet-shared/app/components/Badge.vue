<script setup lang="ts">
import type { StatusTone } from '../types/models'

const props = withDefaults(defineProps<{
  tone?: StatusTone
  dot?: boolean
  size?: 'sm' | 'md'
}>(), {
  tone: 'gray',
  dot: false,
  size: 'md',
})

const tones: Record<StatusTone, string> = {
  green: 'bg-emerald-500/12 text-emerald-700 dark:text-emerald-300 ring-emerald-500/25',
  amber: 'bg-amber-500/12 text-amber-700 dark:text-amber-300 ring-amber-500/25',
  red: 'bg-rose-500/12 text-rose-700 dark:text-rose-300 ring-rose-500/25',
  blue: 'bg-sky-500/12 text-sky-700 dark:text-sky-300 ring-sky-500/25',
  gray: 'bg-slate-500/12 text-slate-600 dark:text-slate-300 ring-slate-500/20',
  purple: 'bg-violet-500/12 text-violet-700 dark:text-violet-300 ring-violet-500/25',
}

const dotColors: Record<StatusTone, string> = {
  green: 'bg-emerald-500',
  amber: 'bg-amber-500',
  red: 'bg-rose-500',
  blue: 'bg-sky-500',
  gray: 'bg-slate-400',
  purple: 'bg-violet-500',
}
</script>

<template>
  <span
    class="inline-flex items-center gap-1.5 rounded-full font-semibold whitespace-nowrap ring-1 ring-inset"
    :class="[tones[props.tone], size === 'sm' ? 'px-2 py-0.5 text-[11px]' : 'px-2.5 py-1 text-xs']"
  >
    <span v-if="dot" class="h-1.5 w-1.5 rounded-full" :class="dotColors[props.tone]" />
    <slot />
  </span>
</template>
