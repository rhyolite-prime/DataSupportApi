<script setup lang="ts">
export interface TableColumn {
  key: string
  label: string
  sortable?: boolean
  align?: 'left' | 'right' | 'center'
  class?: string
}

const props = withDefaults(defineProps<{
  columns: TableColumn[]
  rows: Record<string, any>[]
  rowKey?: string
  loading?: boolean
  empty?: string
  clickable?: boolean
}>(), {
  rowKey: 'id',
  loading: false,
  empty: 'No matching records found.',
  clickable: false,
})

defineEmits<{ rowClick: [row: any] }>()

const sortKey = ref<string>('')
const sortDir = ref<'asc' | 'desc'>('asc')

function toggleSort(col: TableColumn) {
  if (!col.sortable) return
  if (sortKey.value === col.key) {
    sortDir.value = sortDir.value === 'asc' ? 'desc' : 'asc'
  } else {
    sortKey.value = col.key
    sortDir.value = 'asc'
  }
}

const sorted = computed(() => {
  if (!sortKey.value) return props.rows
  const k = sortKey.value
  const dir = sortDir.value === 'asc' ? 1 : -1
  return [...props.rows].sort((a, b) => {
    const va = a[k]
    const vb = b[k]
    if (va == null && vb == null) return 0
    if (va == null) return 1
    if (vb == null) return -1
    if (typeof va === 'number' && typeof vb === 'number') return (va - vb) * dir
    return String(va).localeCompare(String(vb)) * dir
  })
})
</script>

<template>
  <div class="overflow-x-auto">
    <table class="w-full border-collapse text-left text-sm">
      <thead>
        <tr class="border-b border-line bg-canvas/55 text-[11px] font-bold uppercase tracking-wider text-ink-faint">
          <th
            v-for="col in columns"
            :key="col.key"
            class="whitespace-nowrap px-4 py-3 select-none"
            :class="[
              col.sortable ? 'cursor-pointer hover:text-ink' : '',
              col.align === 'right' ? 'text-right' : col.align === 'center' ? 'text-center' : '',
            ]"
            @click="toggleSort(col)"
          >
            <span class="inline-flex items-center gap-1">
              {{ col.label }}
              <AppIcon
                v-if="col.sortable && sortKey === col.key"
                name="chevron-down"
                :size="12"
                class="transition-transform"
                :class="sortDir === 'desc' ? 'rotate-180' : ''"
              />
            </span>
          </th>
        </tr>
      </thead>
      <tbody>
        <tr v-if="loading" v-for="i in 5" :key="`sk-${i}`" class="border-b border-line/60">
          <td v-for="col in columns" :key="col.key" class="px-4 py-3.5">
            <div
              class="h-3.5 animate-pulse rounded bg-canvas"
              :style="{ width: `${45 + ((i * 17 + col.key.length * 9) % 45)}%` }"
            />
          </td>
        </tr>
        <template v-else>
          <tr
            v-for="(row, idx) in sorted"
            :key="row[rowKey] ?? idx"
            class="border-b border-line/60 transition-colors hover:bg-primary-soft/35"
            :class="clickable ? 'cursor-pointer' : ''"
            @click="$emit('rowClick', row)"
          >
            <td
              v-for="col in columns"
              :key="col.key"
              class="whitespace-nowrap px-4 py-3.5 text-ink-soft"
              :class="[
                col.align === 'right' ? 'text-right tnum' : col.align === 'center' ? 'text-center' : '',
                col.class ?? '',
              ]"
            >
              <slot name="cell" :col="col" :row="row" :value="row[col.key]">
                {{ row[col.key] }}
              </slot>
            </td>
          </tr>
          <tr v-if="!sorted.length">
            <td :colspan="columns.length" class="px-4 py-12 text-center text-sm text-ink-faint">
              <AppIcon name="information-circle" :size="22" class="mx-auto mb-2 text-ink-faint" />
              {{ empty }}
            </td>
          </tr>
        </template>
      </tbody>
    </table>
  </div>
</template>
