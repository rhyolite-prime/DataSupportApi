export interface QueuedOfflineSale {
  id: string
  planId: string
  planCode: string
  planName: string
  price: number
  zoneId: string
  source: 'stock' | 'float'
  customerMsisdn: string
  customerName: string
  queuedAt: string
}

const STORAGE_KEY = 'mynet.agent.offlineQueue.v1'

export function useOfflineSalesQueue() {
  const queue = useState<QueuedOfflineSale[]>('mynet-agent-offline-queue', () => [])
  const simulateOffline = useState<boolean>('mynet-agent-sim-offline', () => false)

  function loadQueue() {
    if (!import.meta.client) return
    try {
      const raw = localStorage.getItem(STORAGE_KEY)
      if (raw) queue.value = JSON.parse(raw)
    } catch {
      queue.value = []
    }
  }

  function persist() {
    if (!import.meta.client) return
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(queue.value))
    } catch {
      // ignore storage errors
    }
  }

  function enqueueSale(item: Omit<QueuedOfflineSale, 'id' | 'queuedAt'>) {
    const entry: QueuedOfflineSale = {
      ...item,
      id: `off-${Date.now()}-${Math.random().toString(36).slice(2, 6)}`,
      queuedAt: new Date().toISOString(),
    }
    queue.value.unshift(entry)
    persist()
    return entry
  }

  function removeQueued(id: string) {
    queue.value = queue.value.filter(i => i.id !== id)
    persist()
  }

  async function syncAll(api: ReturnType<typeof useMyNetApi>) {
    const synced: any[] = []
    const remaining: QueuedOfflineSale[] = []
    for (const item of queue.value) {
      try {
        const res = await api.post('/agent/sell', {
          planId: item.planId,
          zoneId: item.zoneId,
          source: item.source,
          customerMsisdn: item.customerMsisdn,
          customerName: item.customerName,
        })
        synced.push(res)
      } catch {
        remaining.push(item)
      }
    }
    queue.value = remaining
    persist()
    return { syncedCount: synced.length, remainingCount: remaining.length, lastResult: synced[0] || null }
  }

  return {
    queue,
    simulateOffline,
    loadQueue,
    enqueueSale,
    removeQueued,
    syncAll,
  }
}
