<script setup lang="ts">
import type { ECharts, EChartsCoreOption } from 'echarts/core'

const props = withDefaults(defineProps<{
  option: Record<string, any>
  height?: string | number
  width?: string | number
  ariaLabel?: string
}>(), {
  height: '220px',
  width: '100%',
  ariaLabel: 'Data visualization chart',
})

const element = ref<HTMLDivElement | null>(null)
let chart: ECharts | null = null
let resizeObserver: ResizeObserver | null = null

async function renderChart() {
  if (!import.meta.client || !element.value) return
  if (!chart) {
    const echarts = await import('echarts/core')
    const { BarChart, LineChart, PieChart } = await import('echarts/charts')
    const { GridComponent, LegendComponent, TooltipComponent } = await import('echarts/components')
    const { CanvasRenderer } = await import('echarts/renderers')
    echarts.use([BarChart, LineChart, PieChart, GridComponent, LegendComponent, TooltipComponent, CanvasRenderer])
    chart = echarts.init(element.value)
  }
  chart.setOption(props.option as EChartsCoreOption, true)
}

function resizeChart() {
  chart?.resize()
}

watch(() => props.option, () => {
  void renderChart()
}, { deep: true })

onMounted(async () => {
  await renderChart()
  if (element.value && typeof ResizeObserver !== 'undefined') {
    resizeObserver = new ResizeObserver(resizeChart)
    resizeObserver.observe(element.value)
  }
  window.addEventListener('resize', resizeChart)
})

onBeforeUnmount(() => {
  resizeObserver?.disconnect()
  if (import.meta.client) window.removeEventListener('resize', resizeChart)
  chart?.dispose()
  chart = null
})
</script>

<template>
  <div
    ref="element"
    class="min-w-0"
    :style="{
      width: typeof width === 'number' ? `${width}px` : width,
      height: typeof height === 'number' ? `${height}px` : height,
    }"
    role="img"
    aria-roledescription="chart"
    :aria-label="ariaLabel"
  />
</template>
