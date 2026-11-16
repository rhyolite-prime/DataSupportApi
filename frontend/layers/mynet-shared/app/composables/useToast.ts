export interface ToastMessage {
  id: number
  tone: 'success' | 'error' | 'info'
  title: string
  message?: string
}

let toastSeq = 0

export function useToast() {
  const toasts = useState<ToastMessage[]>('mynet-toasts', () => [])

  function push(tone: ToastMessage['tone'], title: string, message?: string, ttl = 4500) {
    const id = ++toastSeq
    toasts.value.push({ id, tone, title, message })
    if (import.meta.client) {
      setTimeout(() => dismiss(id), ttl)
    }
  }

  function dismiss(id: number) {
    toasts.value = toasts.value.filter(t => t.id !== id)
  }

  return {
    toasts,
    success: (title: string, message?: string) => push('success', title, message),
    error: (title: string, message?: string) => push('error', title, message, 6500),
    info: (title: string, message?: string) => push('info', title, message),
    dismiss,
  }
}
