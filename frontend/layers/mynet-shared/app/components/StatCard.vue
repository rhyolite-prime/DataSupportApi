<script setup lang="ts">
const props = withDefaults(defineProps<{
  label: string
  value: string | number
  icon: string
  tone?: 'primary' | 'green' | 'amber' | 'red' | 'blue' | 'purple'
  sub?: string
  trend?: string
  trendUp?: boolean | null
}>(), {
  tone: 'primary',
  trendUp: null,
})

const toneCls: Record<string, string> = {
  primary: 'bg-primary-soft text-primary',
  green: 'bg-emerald-500/12 text-emerald-600 dark:text-emerald-400',
  amber: 'bg-amber-500/12 text-amber-600 dark:text-amber-400',
  red: 'bg-rose-500/12 text-rose-600 dark:text-rose-400',
  blue: 'bg-sky-500/12 text-sky-600 dark:text-sky-400',
  purple: 'bg-violet-500/12 text-violet-600 dark:text-violet-400',
}
</script>

<template>
  <BaseCard :pad="false">
    <div class="flex items-start gap-4 p-5">
      <span class="flex h-11 w-11 shrink-0 items-center justify-center rounded-xl" :class="toneCls[props.tone]">
        <AppIcon :name="icon" :size="22" />
      </span>
      <div class="min-w-0 flex-1">
        <p class="text-[11px] font-semibold uppercase tracking-wider text-ink-faint">{{ label }}</p>
        <p class="mt-1 truncate text-xl font-bold text-ink tnum">{{ value }}</p>
        <div v-if="sub || trend" class="mt-1 flex flex-wrap items-center gap-2">
          <span
            v-if="trend"
            class="inline-flex items-center gap-1 text-[11px] font-semibold"
            :class="trendUp === null ? 'text-ink-faint' : trendUp ? 'text-emerald-600 dark:text-emerald-400' : 'text-rose-600 dark:text-rose-400'"
          >
            <AppIcon v-if="trendUp !== null" :name="trendUp ? 'arrow-trending-up' : 'arrow-trending-down'" :size="13" />
            {{ trend }}
          </span>
          <span v-if="sub" class="truncate text-xs text-ink-faint">{{ sub }}</span>
        </div>
      </div>
    </div>
  </BaseCard>
</template>
