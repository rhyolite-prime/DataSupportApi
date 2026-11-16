<script setup lang="ts">
import type { PortalNavigationGroup, PortalUserSession } from '../types/models'

const props = withDefaults(defineProps<{
  mobileOpen: boolean
  workspaceLabel: string
  dashboardTo?: string
  navigationGroups: PortalNavigationGroup[]
  user: PortalUserSession | null
  footerSubtitle?: string
}>(), {
  dashboardTo: '/',
  footerSubtitle: 'MyNet · Powered by Magmanet',
})

const emit = defineEmits<{ close: []; logout: [] }>()
const route = useRoute()

const displayName = computed(() => props.user?.name || 'MyNet Operator')
const roleSubtitle = computed(() => {
  if (!props.user) return 'Sandbox Demo Mode'
  if (props.user.portal === 'agent') {
    return `${props.user.code || 'Agent'} · ${props.user.territoryZoneName || 'Field Territory'}`
  }
  return `${props.user.role} · ${props.user.tenantName}`
})
const avatarInitials = computed(() => initialsFromName(displayName.value))

const isDashboardActive = computed(() => route.path === props.dashboardTo)
</script>

<template>
  <Transition name="fade">
    <div
      v-if="props.mobileOpen"
      class="no-print fixed inset-0 z-40 bg-slate-950/60 backdrop-blur-[2px] lg:hidden"
      @click="emit('close')"
    />
  </Transition>

  <aside
    class="no-print fixed inset-y-0 left-0 z-50 flex w-[264px] flex-col bg-sidebar text-white/75 transition-transform duration-200 lg:translate-x-0"
    :class="props.mobileOpen ? 'translate-x-0' : '-translate-x-full lg:translate-x-0'"
  >
    <div class="border-b border-white/10 px-5 py-5">
      <NuxtLink :to="props.dashboardTo" class="block min-w-0" @click="emit('close')">
        <BrandMark dark :product-name="props.workspaceLabel" />
      </NuxtLink>
    </div>

    <nav class="flex-1 space-y-5 overflow-y-auto px-3 py-4">
      <NuxtLink
        :to="props.dashboardTo"
        class="flex items-center gap-3 rounded-xl px-3 py-2.5 text-sm font-medium transition-colors"
        :class="isDashboardActive ? 'nav-active font-semibold' : 'text-white/70 hover:bg-white/6 hover:text-white'"
        @click="emit('close')"
      >
        <AppIcon name="home" :size="18" />
        <span>Overview</span>
      </NuxtLink>

      <section v-for="group in props.navigationGroups" :key="group.title">
        <p class="mb-1.5 flex items-center gap-2 px-3 text-[10px] font-bold uppercase tracking-[0.13em] text-white/40">
          <AppIcon :name="group.icon" :size="13" />
          {{ group.title }}
        </p>
        <div class="space-y-0.5">
          <PortalSidebarNavItem
            v-for="link in group.links"
            :key="link.label"
            :item="link"
            @navigate="emit('close')"
          />
        </div>
      </section>
    </nav>

    <div class="border-t border-white/10 px-4 py-3.5">
      <div class="flex min-w-0 items-center gap-3 rounded-xl bg-white/5 px-2.5 py-2">
        <span
          class="flex h-9 w-9 shrink-0 items-center justify-center rounded-full text-xs font-extrabold text-slate-950"
          style="background: var(--brand-accent)"
        >
          {{ avatarInitials }}
        </span>
        <span class="min-w-0 flex-1">
          <span class="block truncate text-xs font-semibold text-white">{{ displayName }}</span>
          <span class="mt-0.5 block truncate text-[10px] text-white/50">{{ roleSubtitle }}</span>
        </span>
        <button
          type="button"
          class="cursor-pointer rounded-lg p-1.5 text-white/55 transition hover:bg-white/12 hover:text-white"
          aria-label="Sign out"
          title="Sign out"
          @click="emit('logout')"
        >
          <AppIcon name="arrow-left-on-rectangle" :size="17" />
        </button>
      </div>
      <p class="px-2 pt-2 text-[10px] leading-relaxed text-white/35">{{ props.footerSubtitle }}</p>
    </div>
  </aside>
</template>
