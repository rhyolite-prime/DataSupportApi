<script setup lang="ts">
import type { PortalNavigationLink } from '../types/models'

defineOptions({ name: 'PortalSidebarNavItem' })

const props = withDefaults(defineProps<{
  item: PortalNavigationLink
  level?: number
}>(), {
  level: 0,
})
const emit = defineEmits<{ navigate: [] }>()
const route = useRoute()
const expanded = ref(false)

const hasChildren = computed(() => Boolean(props.item.children?.length))

function isCurrent(item: PortalNavigationLink): boolean {
  if (item.to) {
    if (item.to === '/') return route.path === '/'
    if (route.path === item.to || route.path.startsWith(`${item.to}/`)) return true
  }
  return item.children?.some(isCurrent) ?? false
}

const isActive = computed(() => isCurrent(props.item))

watch(isActive, (active) => {
  if (active && hasChildren.value) expanded.value = true
}, { immediate: true })
</script>

<template>
  <div>
    <button
      v-if="hasChildren"
      type="button"
      class="flex w-full cursor-pointer items-center gap-3 rounded-xl px-3 py-2.5 text-left text-sm font-medium transition-colors"
      :class="isActive ? 'bg-white/8 text-white' : 'text-white/70 hover:bg-white/6 hover:text-white'"
      :aria-expanded="expanded"
      @click="expanded = !expanded"
    >
      <AppIcon :name="item.icon" :size="level === 0 ? 18 : 16" />
      <span class="min-w-0 flex-1 truncate">{{ item.label }}</span>
      <AppIcon name="chevron-down" :size="14" class="transition-transform" :class="expanded ? 'rotate-180' : ''" />
    </button>

    <NuxtLink
      v-else-if="item.to && !item.comingSoon"
      :to="item.to"
      class="flex items-center gap-3 rounded-xl font-medium transition-colors"
      :class="[
        level === 0 ? 'px-3 py-2.5 text-sm' : 'px-2.5 py-2 text-[13px]',
        isActive ? 'nav-active font-semibold' : 'text-white/70 hover:bg-white/6 hover:text-white',
      ]"
      :aria-current="isActive ? 'page' : undefined"
      @click="emit('navigate')"
    >
      <AppIcon :name="item.icon" :size="level === 0 ? 18 : 16" />
      <span class="min-w-0 flex-1 truncate">{{ item.label }}</span>
      <span
        v-if="item.badge !== undefined && item.badge !== 0"
        class="rounded-full px-1.5 py-0.5 text-[10px] font-bold text-slate-950 tnum"
        style="background: var(--brand-accent)"
      >
        {{ item.badge }}
      </span>
    </NuxtLink>

    <div
      v-if="hasChildren && expanded"
      class="mt-1 ml-3 space-y-0.5 border-l border-white/12 pl-2"
    >
      <PortalSidebarNavItem
        v-for="child in item.children"
        :key="child.label"
        :item="child"
        :level="level + 1"
        @navigate="emit('navigate')"
      />
    </div>
  </div>
</template>
