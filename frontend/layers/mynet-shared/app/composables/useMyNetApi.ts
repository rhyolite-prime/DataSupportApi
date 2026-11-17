import type { PortalUserSession } from '../types/models'

/**
 * Unified API & session client shared by MyNet Admin and MyNet Agent portals.
 * All browser requests stay strictly same-origin under `/api/mynet/*` so the
 * portals work identically behind localhost, reverse proxies, or Arena live
 * preview sandboxes (`https://{port}-{sandboxId}.e2b.app`).
 *
 * Today, Nitro endpoints serve stateful mock data modeled 1:1 on MagmaNet's
 * DataSupportApi DTOs. When backend integration starts, setting
 * `NUXT_MAGMA_API_BASE_URL` forwards server-side without changing Vue pages.
 */
export function useMyNetApi() {
  async function request<T = any>(path: string, opts: Record<string, any> = {}): Promise<T> {
    try {
      const response = await $fetch<any>(`/api/mynet${path}`, opts)
      if (response && typeof response === 'object' && 'result' in response && 'success' in response) {
        if (response.success === false) {
          throw new Error(response.message || 'Operation could not be completed.')
        }
        return response.result as T
      }
      return response as T
    } catch (e: any) {
      const msg =
        e?.data?.message ||
        e?.data?.statusMessage ||
        e?.statusMessage ||
        e?.message ||
        'Request failed. Please try again.'
      throw new Error(msg)
    }
  }

  return {
    get: <T = any>(path: string, query?: Record<string, any>) =>
      request<T>(path, { method: 'GET', query }),
    post: <T = any>(path: string, body?: Record<string, any>) =>
      request<T>(path, { method: 'POST', body }),
    put: <T = any>(path: string, body?: Record<string, any>) =>
      request<T>(path, { method: 'PUT', body }),
    del: <T = any>(path: string) =>
      request<T>(path, { method: 'DELETE' }),
  }
}

export function usePortalSession(portalKind: 'admin' | 'agent') {
  const session = useState<PortalUserSession | null>(`mynet-session-${portalKind}`, () => null)
  const api = useMyNetApi()

  async function refreshSession() {
    try {
      const data = await api.get<PortalUserSession>('/auth/session', { portal: portalKind })
      session.value = data
      return data
    } catch {
      session.value = null
      return null
    }
  }

  async function loginAdmin(payload: { mode: 'pin' | 'apikey'; msisdn?: string; pin?: string; clientId?: string; clientSecret?: string }) {
    const data = await api.post<PortalUserSession>('/auth/admin-login', payload)
    session.value = data
    return data
  }

  async function loginAgent(payload: { agentIdentifier: string; pin: string }) {
    const data = await api.post<PortalUserSession>('/auth/agent-login', payload)
    session.value = data
    return data
  }

  async function logout() {
    await api.post('/auth/logout', { portal: portalKind }).catch(() => {})
    session.value = null
    await navigateTo('/login')
  }

  async function resetDemoData() {
    await api.post('/demo/reset', {})
  }

  return {
    session,
    refreshSession,
    loginAdmin,
    loginAgent,
    logout,
    resetDemoData,
  }
}
