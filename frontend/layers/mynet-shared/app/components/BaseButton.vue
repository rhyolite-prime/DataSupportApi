<script setup lang="ts">
const props = withDefaults(defineProps<{
  variant?: 'primary' | 'secondary' | 'ghost' | 'danger' | 'success' | 'accent'
  size?: 'sm' | 'md' | 'lg'
  icon?: string
  to?: string
  block?: boolean
  disabled?: boolean
  loading?: boolean
  type?: 'button' | 'submit'
}>(), {
  variant: 'primary',
  size: 'md',
  type: 'button',
  loading: false,
})

const cls = computed(() => [
  'inline-flex items-center justify-center gap-2 font-semibold rounded-xl transition-all cursor-pointer',
  'focus:outline-none focus-visible:ring-2 focus-visible:ring-offset-1 focus-visible:ring-primary/50',
  'disabled:opacity-50 disabled:pointer-events-none',
  props.size === 'sm'
    ? 'text-xs px-3 py-1.5'
    : props.size === 'lg'
      ? 'text-sm px-5 py-3'
      : 'text-sm px-4 py-2.5',
  props.block ? 'w-full' : '',
  {
    primary: 'bg-primary text-on-primary hover:bg-primary-dark shadow-sm',
    secondary: 'bg-surface text-ink border border-line hover:border-primary/40 hover:bg-canvas',
    ghost: 'text-ink-soft hover:bg-canvas hover:text-ink',
    danger: 'bg-rose-600 text-white hover:bg-rose-700 shadow-sm',
    success: 'bg-emerald-600 text-white hover:bg-emerald-700 shadow-sm',
    accent: 'text-slate-950 shadow-sm hover:opacity-95',
  }[props.variant],
])
</script>

<template>
  <NuxtLink
    v-if="to && !disabled"
    :to="to"
    :class="cls"
    :style="variant === 'accent' ? { background: 'var(--brand-accent)' } : undefined"
  >
    <AppIcon v-if="loading" name="arrow-path" :size="size === 'sm' ? 14 : 16" class="animate-spin" />
    <AppIcon v-else-if="icon" :name="icon" :size="size === 'sm' ? 15 : 17" />
    <slot />
  </NuxtLink>
  <button
    v-else
    :type="type"
    :disabled="disabled || loading"
    :class="cls"
    :style="variant === 'accent' ? { background: 'var(--brand-accent)' } : undefined"
  >
    <AppIcon v-if="loading" name="arrow-path" :size="size === 'sm' ? 14 : 16" class="animate-spin" />
    <AppIcon v-else-if="icon" :name="icon" :size="size === 'sm' ? 15 : 17" />
    <slot />
  </button>
</template>
