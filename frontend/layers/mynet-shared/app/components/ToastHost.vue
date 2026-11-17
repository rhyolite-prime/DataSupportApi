<script setup lang="ts">
const { toasts, dismiss } = useToast()
const toneCls = {
  success: { bar: 'bg-emerald-500', icon: 'check-circle', text: 'text-emerald-500' },
  error: { bar: 'bg-rose-500', icon: 'x-circle', text: 'text-rose-500' },
  info: { bar: 'bg-sky-500', icon: 'information-circle', text: 'text-sky-500' },
} as const
</script>

<template>
  <Teleport to="body">
    <div class="no-print fixed right-5 bottom-5 z-[100] flex w-[min(92vw,380px)] flex-col gap-2">
      <TransitionGroup name="pop">
        <div
          v-for="t in toasts"
          :key="t.id"
          class="flex items-stretch overflow-hidden rounded-xl border border-line bg-surface shadow-xl"
        >
          <span class="w-1.5" :class="toneCls[t.tone].bar" />
          <div class="flex flex-1 items-start gap-3 p-3.5">
            <AppIcon :name="toneCls[t.tone].icon" :size="20" :class="toneCls[t.tone].text" />
            <div class="min-w-0 flex-1">
              <p class="text-sm font-semibold text-ink">{{ t.title }}</p>
              <p v-if="t.message" class="mt-0.5 break-words text-xs text-ink-faint">{{ t.message }}</p>
            </div>
            <button type="button" class="cursor-pointer text-ink-faint hover:text-ink" @click="dismiss(t.id)">
              <AppIcon name="x-mark" :size="15" />
            </button>
          </div>
        </div>
      </TransitionGroup>
    </div>
  </Teleport>
</template>
