<script setup lang="ts">
const props = defineProps<{
  open: boolean
  title: string
  message: string
  confirmLabel?: string
  tone?: 'primary' | 'danger'
  icon?: string
  loading?: boolean
}>()
const emit = defineEmits<{ confirm: []; cancel: [] }>()
</script>

<template>
  <ModalShell :open="open" :title="title" width="max-w-md" @close="emit('cancel')">
    <div class="flex gap-4">
      <span
        class="flex h-11 w-11 shrink-0 items-center justify-center rounded-full"
        :class="props.tone === 'danger' ? 'bg-rose-500/12 text-rose-600' : 'bg-primary-soft text-primary'"
      >
        <AppIcon :name="props.icon ?? 'exclamation-triangle'" :size="22" />
      </span>
      <p class="pt-1.5 text-sm leading-relaxed text-ink-soft">{{ message }}</p>
    </div>
    <template #footer>
      <BaseButton variant="secondary" size="sm" @click="emit('cancel')">Cancel</BaseButton>
      <BaseButton
        :variant="props.tone === 'danger' ? 'danger' : 'primary'"
        size="sm"
        :loading="props.loading"
        @click="emit('confirm')"
      >
        {{ props.confirmLabel ?? 'Confirm' }}
      </BaseButton>
    </template>
  </ModalShell>
</template>
