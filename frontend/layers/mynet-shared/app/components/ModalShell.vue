<script setup lang="ts">
const props = withDefaults(defineProps<{
  open: boolean
  title: string
  subtitle?: string
  width?: string
}>(), {
  width: 'max-w-lg',
})
const emit = defineEmits<{ close: [] }>()

function onKey(e: KeyboardEvent) {
  if (e.key === 'Escape' && props.open) emit('close')
}
onMounted(() => window.addEventListener('keydown', onKey))
onUnmounted(() => window.removeEventListener('keydown', onKey))
</script>

<template>
  <Teleport to="body">
    <Transition name="fade">
      <div v-if="open" class="fixed inset-0 z-50 flex items-start justify-center overflow-y-auto p-4 sm:p-8">
        <div class="fixed inset-0 bg-slate-950/55 backdrop-blur-[2px]" @click="emit('close')" />
        <Transition name="pop" appear>
          <div v-if="open" class="relative my-6 w-full" :class="props.width">
            <div class="rounded-2xl border border-line bg-surface shadow-2xl">
              <header class="flex items-start justify-between gap-4 border-b border-line px-6 py-4">
                <div>
                  <h2 class="text-base font-bold text-ink">{{ title }}</h2>
                  <p v-if="subtitle" class="mt-0.5 text-xs text-ink-faint">{{ subtitle }}</p>
                </div>
                <button
                  type="button"
                  class="-m-1 cursor-pointer rounded-lg p-1.5 text-ink-faint transition hover:bg-canvas hover:text-ink"
                  aria-label="Close dialog"
                  @click="emit('close')"
                >
                  <AppIcon name="x-mark" :size="18" />
                </button>
              </header>
              <div class="px-6 py-5">
                <slot />
              </div>
              <footer
                v-if="$slots.footer"
                class="flex flex-wrap items-center justify-end gap-2 rounded-b-2xl border-t border-line bg-canvas/60 px-6 py-4"
              >
                <slot name="footer" />
              </footer>
            </div>
          </div>
        </Transition>
      </div>
    </Transition>
  </Teleport>
</template>
