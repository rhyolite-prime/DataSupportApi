<script setup lang="ts">
import type { PortalSearchItem, PortalUserSession } from '../types/models'

const props = withDefaults(defineProps<{
  dark: boolean
  workspaceLabel: string
  pageTitle: string
  searchPlaceholder?: string
  searchItems: PortalSearchItem[]
  user: PortalUserSession | null
  alertCount?: number
  alertsTo?: string
}>(), {
  searchPlaceholder: 'Search MyNet pages, zones, vouchers…',
  alertCount: 0,
  alertsTo: undefined,
})

const emit = defineEmits<{
  toggleMenu: []
  toggleTheme: []
  logout: []
  resetDemo: []
}>()

const router = useRouter()
const { presets, applyPreset, activePreset } = useBrand()

const searchQuery = ref('')
const searchFocused = ref(false)
const showPalette = ref(false)
const showUser = ref(false)
const searchInput = ref<HTMLInputElement | null>(null)

const searchResults = computed(() => {
  const q = searchQuery.value.trim().toLowerCase()
  if (!q) return []
  return props.searchItems.filter(item => `${item.label} ${item.detail}`.toLowerCase().includes(q)).slice(0, 8)
})

const displayName = computed(() => props.user?.name || 'MyNet Operator')
const avatarInitials = computed(() => initialsFromName(displayName.value))

function go(to: string) {
  searchQuery.value = ''
  searchFocused.value = false
  showPalette.value = false
  showUser.value = false
  router.push(to)
}

function closeAll() {
  searchFocused.value = false
  showPalette.value = false
  showUser.value = false
}

function onDocClick(e: MouseEvent) {
  if (!(e.target as HTMLElement).closest('[data-popover]')) closeAll()
}

onMounted(() => document.addEventListener('click', onDocClick))
onBeforeUnmount(() => document.removeEventListener('click', onDocClick))
</script>

<template>
  <header class="no-print sticky top-0 z-30 border-b border-line bg-surface/90 backdrop-blur">
    <div class="flex h-16 items-center gap-3 px-4 sm:px-6">
      <button
        type="button"
        class="-ml-1.5 cursor-pointer rounded-xl p-2 text-ink-soft transition hover:bg-canvas lg:hidden"
        aria-label="Open navigation"
        @click="emit('toggleMenu')"
      >
        <AppIcon name="bars-3" :size="21" />
      </button>

      <div class="hidden shrink-0 items-center gap-2 text-xs text-ink-faint lg:flex">
        <span>{{ props.workspaceLabel }}</span>
        <span class="text-ink-faint/50">/</span>
        <span class="font-semibold text-ink">{{ props.pageTitle }}</span>
      </div>

      <!-- Command / Quick Search -->
      <div class="relative mx-auto min-w-0 max-w-xl flex-1" data-popover>
        <div class="relative">
          <AppIcon name="magnifying-glass" :size="17" class="absolute top-1/2 left-3.5 -translate-y-1/2 text-ink-faint" />
          <input
            ref="searchInput"
            v-model="searchQuery"
            type="search"
            autocomplete="off"
            :placeholder="props.searchPlaceholder"
            class="w-full rounded-full border border-transparent bg-canvas py-2 pl-10 pr-9 text-sm text-ink placeholder:text-ink-faint transition focus:border-primary/35 focus:bg-surface focus:outline-none focus:ring-2 focus:ring-primary/15"
            @focus="searchFocused = true"
            @keydown.esc="searchQuery = ''; searchFocused = false"
            @keydown.enter.prevent="searchResults[0] && go(searchResults[0].to)"
          >
          <button
            v-if="searchQuery"
            type="button"
            class="absolute top-1/2 right-3 -translate-y-1/2 cursor-pointer text-ink-faint hover:text-ink"
            aria-label="Clear search"
            @click="searchQuery = ''; searchInput?.focus()"
          >
            <AppIcon name="x-mark" :size="15" />
          </button>
        </div>

        <Transition name="pop">
          <div
            v-if="searchFocused && searchQuery.trim()"
            class="absolute right-0 left-0 mt-2 overflow-hidden rounded-2xl border border-line bg-surface shadow-xl"
          >
            <p v-if="!searchResults.length" class="px-4 py-6 text-center text-xs text-ink-faint">
              No workspaces match “{{ searchQuery }}”.
            </p>
            <div v-else class="py-2">
              <p class="px-4 pt-1 pb-1 text-[10px] font-bold uppercase tracking-wider text-ink-faint">
                Quick jump
              </p>
              <button
                v-for="item in searchResults"
                :key="item.to"
                type="button"
                class="flex w-full cursor-pointer items-center gap-3 px-4 py-2.5 text-left transition hover:bg-primary-soft/60"
                @click="go(item.to)"
              >
                <span v-if="item.icon" class="flex h-8 w-8 shrink-0 items-center justify-center rounded-lg bg-primary-soft text-primary">
                  <AppIcon :name="item.icon" :size="15" />
                </span>
                <span class="min-w-0 flex-1">
                  <span class="block truncate text-sm font-semibold text-ink">{{ item.label }}</span>
                  <span class="mt-0.5 block truncate text-[11px] text-ink-faint">{{ item.detail }}</span>
                </span>
                <AppIcon name="arrow-right" :size="14" class="shrink-0 text-ink-faint" />
              </button>
            </div>
          </div>
        </Transition>
      </div>

      <!-- Right Action Controls -->
      <div class="ml-auto flex shrink-0 items-center gap-1 sm:gap-1.5">
        <span class="hidden items-center gap-1.5 rounded-full border border-emerald-500/25 bg-emerald-500/10 px-2.5 py-1 text-[11px] font-semibold text-emerald-700 dark:text-emerald-300 xl:inline-flex">
          <span class="h-1.5 w-1.5 rounded-full bg-emerald-500 animate-pulse" />
          Mock API Ready
        </span>

        <!-- Alerts bell -->
        <NuxtLink
          v-if="props.alertsTo"
          :to="props.alertsTo"
          class="relative rounded-xl p-2 text-ink-soft transition hover:bg-canvas hover:text-ink"
          title="Active alerts"
        >
          <AppIcon name="bell" :size="19" />
          <span
            v-if="props.alertCount > 0"
            class="absolute top-1 right-1 flex h-4 min-w-4 items-center justify-center rounded-full bg-rose-600 px-1 text-[10px] font-bold text-white tnum"
          >
            {{ props.alertCount }}
          </span>
        </NuxtLink>

        <!-- White-label Theme Switcher -->
        <div class="relative" data-popover>
          <button
            type="button"
            class="cursor-pointer rounded-xl p-2 text-ink-soft transition hover:bg-canvas hover:text-ink"
            title="White-label brand palette"
            aria-label="White-label brand palette"
            @click="showPalette = !showPalette; showUser = false"
          >
            <AppIcon name="sparkles" :size="19" />
          </button>
          <Transition name="pop">
            <div
              v-if="showPalette"
              class="absolute right-0 mt-2 w-72 rounded-2xl border border-line bg-surface p-3 shadow-xl"
            >
              <p class="px-2 pb-2 text-[10px] font-bold uppercase tracking-wider text-ink-faint">
                MyNet White-Label Theme
              </p>
              <button
                v-for="preset in presets"
                :key="preset.key"
                type="button"
                class="flex w-full cursor-pointer items-center gap-3 rounded-xl px-2.5 py-2 text-left transition hover:bg-canvas"
                @click="applyPreset(preset); showPalette = false"
              >
                <span class="flex -space-x-1.5">
                  <span class="h-5 w-5 rounded-full ring-2 ring-surface" :style="{ background: preset.theme.primary }" />
                  <span class="h-5 w-5 rounded-full ring-2 ring-surface" :style="{ background: preset.theme.accent }" />
                </span>
                <span class="flex-1 text-xs font-semibold text-ink">{{ preset.label }}</span>
                <AppIcon v-if="activePreset === preset.key" name="check" :size="15" class="text-primary" />
              </button>
            </div>
          </Transition>
        </div>

        <!-- Dark / Light mode toggle -->
        <button
          type="button"
          class="cursor-pointer rounded-xl p-2 text-ink-soft transition hover:bg-canvas hover:text-ink"
          :title="props.dark ? 'Switch to light mode' : 'Switch to dark mode'"
          :aria-label="props.dark ? 'Switch to light mode' : 'Switch to dark mode'"
          @click="emit('toggleTheme')"
        >
          <AppIcon :name="props.dark ? 'sun' : 'moon'" :size="19" />
        </button>

        <!-- Operator Account Menu -->
        <div class="relative ml-1 border-l border-line pl-2" data-popover>
          <button
            type="button"
            class="flex cursor-pointer items-center gap-2 rounded-xl p-1.5 transition hover:bg-canvas"
            @click="showUser = !showUser; showPalette = false"
          >
            <span class="flex h-8 w-8 items-center justify-center rounded-full bg-primary text-xs font-bold text-on-primary">
              {{ avatarInitials }}
            </span>
            <span class="hidden max-w-[132px] text-left leading-tight sm:block">
              <span class="block truncate text-xs font-semibold text-ink">{{ displayName }}</span>
              <span class="mt-0.5 block truncate text-[10px] text-ink-faint">{{ props.user?.role || 'Operator' }}</span>
            </span>
            <AppIcon name="chevron-down" :size="14" class="hidden text-ink-faint sm:block" />
          </button>

          <Transition name="pop">
            <div
              v-if="showUser"
              class="absolute right-0 mt-2 w-64 overflow-hidden rounded-2xl border border-line bg-surface shadow-xl"
            >
              <div class="border-b border-line bg-canvas/60 px-4 py-3">
                <p class="truncate text-sm font-semibold text-ink">{{ displayName }}</p>
                <p class="mt-0.5 truncate text-[11px] text-ink-faint">
                  {{ props.user?.msisdn }} · {{ props.user?.tenantName || 'MyNet Ghana' }}
                </p>
              </div>
              <button
                type="button"
                class="flex w-full cursor-pointer items-center gap-3 px-4 py-3 text-left text-xs font-semibold text-ink-soft transition hover:bg-primary-soft/50"
                @click="showUser = false; emit('resetDemo')"
              >
                <AppIcon name="arrow-path" :size="16" class="text-primary" />
                Reset demo dataset
              </button>
              <button
                type="button"
                class="flex w-full cursor-pointer items-center gap-3 border-t border-line px-4 py-3 text-left text-xs font-semibold text-rose-600 transition hover:bg-rose-500/10"
                @click="showUser = false; emit('logout')"
              >
                <AppIcon name="arrow-left-on-rectangle" :size="16" />
                Sign out
              </button>
            </div>
          </Transition>
        </div>
      </div>
    </div>
  </header>
</template>
