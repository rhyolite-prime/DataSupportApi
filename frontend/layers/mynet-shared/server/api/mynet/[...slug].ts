import type { H3Event } from 'h3'
import { createError, deleteCookie, getCookie, getMethod, getQuery, readBody, setCookie } from 'h3'
import {
  generateVoucherCode,
  getMockDb,
  nextMockId,
  recomputeAgentStock,
  recomputeBatchCounts,
  resetMockDb,
} from '../../utils/mock-db'
import type { PortalUserSession } from '../../../app/types/models'

function ok<T>(result: T, message = 'OK') {
  return {
    success: true,
    message,
    result,
    error: null,
    targetUrl: '',
  }
}

function getActiveAgentId(event: H3Event, fallbackId = 'agent-1'): string {
  const cookieAgent = getCookie(event, 'mynet_agent_id')
  const db = getMockDb()
  if (cookieAgent && db.agents.some(a => a.id === cookieAgent)) return cookieAgent
  return fallbackId
}

export default defineEventHandler(async (event) => {
  const method = getMethod(event).toUpperCase()
  const rawSlug = event.context.params?.slug || ''
  const path = '/' + String(rawSlug).replace(/^\/+|\/+$/g, '')
  const query = getQuery(event)
  const db = getMockDb()

  // ── Demo dataset reset ─────────────────────────────────────────────────
  if (path === '/demo/reset' && method === 'POST') {
    resetMockDb()
    return ok({ reset: true }, 'Demo data restored to clean baseline.')
  }

  // ── Authentication & Session ───────────────────────────────────────────
  if (path === '/auth/session' && method === 'GET') {
    const portal = String(query.portal || 'admin') as 'admin' | 'agent'
    if (portal === 'agent') {
      const signedOut = getCookie(event, 'mynet_agent_logged_out') === '1'
      if (signedOut) {
        return ok<PortalUserSession | null>(null)
      }
      const agentId = getActiveAgentId(event)
      const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
      return ok<PortalUserSession>({
        authenticated: true,
        portal: 'agent',
        id: agent.id,
        name: agent.name,
        role: `${agent.tier} Reseller`,
        msisdn: agent.msisdn,
        email: agent.email,
        code: agent.agentCode,
        territoryZoneId: agent.territoryZoneId,
        territoryZoneName: agent.territoryZoneName,
        tier: agent.tier,
        commissionRate: agent.commissionRate,
        tenantName: db.business.name,
        businessId: db.business.id,
      })
    }

    const signedOut = getCookie(event, 'mynet_admin_logged_out') === '1'
    if (signedOut) {
      return ok<PortalUserSession | null>(null)
    }
    const staffId = getCookie(event, 'mynet_admin_id') || 'staff-admin'
    const member = db.staff.find(s => s.id === staffId) || db.staff[0]
    return ok<PortalUserSession>({
      authenticated: true,
      portal: 'admin',
      id: member.id,
      name: member.fullName,
      role: member.role,
      msisdn: member.msisdn,
      email: member.email,
      zoneIds: member.zoneIds,
      tenantName: db.business.name,
      businessId: db.business.id,
    })
  }

  if (path === '/auth/admin-login' && method === 'POST') {
    const body = await readBody(event)
    let member = db.staff[0]
    if (body?.mode === 'apikey') {
      if (!body.clientId || !body.clientSecret) {
        throw createError({ statusCode: 400, statusMessage: 'ClientId and ClientSecret are required.' })
      }
    } else {
      const msisdn = String(body?.msisdn || '').trim()
      const pin = String(body?.pin || '').trim()
      if (!msisdn || pin.length < 4) {
        throw createError({ statusCode: 400, statusMessage: 'Enter staff phone number and 4-digit PIN.' })
      }
      const found = db.staff.find(s => s.msisdn === msisdn || s.email.toLowerCase() === msisdn.toLowerCase())
      if (found) member = found
    }
    member.lastLoginAt = new Date().toISOString()
    deleteCookie(event, 'mynet_admin_logged_out', { path: '/' })
    setCookie(event, 'mynet_admin_id', member.id, { path: '/', httpOnly: false, maxAge: 86400 * 7 })
    return ok<PortalUserSession>({
      authenticated: true,
      portal: 'admin',
      id: member.id,
      name: member.fullName,
      role: member.role,
      msisdn: member.msisdn,
      email: member.email,
      zoneIds: member.zoneIds,
      tenantName: db.business.name,
      businessId: db.business.id,
    }, 'Signed in to MyNet Admin.')
  }

  if (path === '/auth/agent-login' && method === 'POST') {
    const body = await readBody(event)
    const identifier = String(body?.agentIdentifier || '').trim()
    const pin = String(body?.pin || '').trim()
    if (!identifier || pin.length < 4) {
      throw createError({ statusCode: 400, statusMessage: 'Enter your registered Agent phone/code and 4-digit PIN.' })
    }
    const found =
      db.agents.find(
        a =>
          a.msisdn === identifier ||
          a.agentCode.toLowerCase() === identifier.toLowerCase() ||
          a.email.toLowerCase() === identifier.toLowerCase(),
      ) || db.agents[0]

    if (found.status === 'SUSPENDED') {
      throw createError({ statusCode: 403, statusMessage: 'This agent wallet is currently suspended. Contact MyNet Operations.' })
    }

    deleteCookie(event, 'mynet_agent_logged_out', { path: '/' })
    setCookie(event, 'mynet_agent_id', found.id, { path: '/', httpOnly: false, maxAge: 86400 * 7 })
    return ok<PortalUserSession>({
      authenticated: true,
      portal: 'agent',
      id: found.id,
      name: found.name,
      role: `${found.tier} Reseller`,
      msisdn: found.msisdn,
      email: found.email,
      code: found.agentCode,
      territoryZoneId: found.territoryZoneId,
      territoryZoneName: found.territoryZoneName,
      tier: found.tier,
      commissionRate: found.commissionRate,
      tenantName: db.business.name,
      businessId: db.business.id,
    }, 'Welcome back to MyNet Agent.')
  }

  if (path === '/auth/logout' && method === 'POST') {
    const body = await readBody(event).catch(() => ({}))
    if (body?.portal === 'agent') {
      setCookie(event, 'mynet_agent_logged_out', '1', { path: '/', httpOnly: false })
    } else {
      setCookie(event, 'mynet_admin_logged_out', '1', { path: '/', httpOnly: false })
    }
    return ok({ signedOut: true })
  }

  // ── Admin: Overview & NOC Dashboard ────────────────────────────────────
  if (path === '/admin/overview' && method === 'GET') {
    const liveZones = db.zones.filter(z => z.status === 'LIVE')
    const onlineDevices = db.devices.filter(d => d.status === 'ONLINE').length
    const activeSubscribers = db.subscribers.filter(s => s.status === 'ACTIVE').length
    const concurrentSessions = db.zones.reduce((sum, z) => sum + z.activeSessions, 0)
    const monthlyRevenue = db.zones.reduce((sum, z) => sum + z.monthlyRevenue, 0)
    const openAlerts = db.alerts.filter(a => !a.resolvedAt)
    const openTickets = db.tickets.filter(t => t.status === 'OPEN' || t.status === 'IN_PROGRESS')
    const availableVouchers = db.vouchers.filter(v => v.status === 'AVAILABLE' || v.status === 'ASSIGNED').length
    const outboxBacklog = db.outbox.filter(o => o.status !== 'DONE').length

    return ok({
      kpis: {
        liveZones: liveZones.length,
        totalZones: db.zones.length,
        onlineDevices,
        totalDevices: db.devices.length,
        activeSubscribers,
        totalSubscribers: db.subscribers.length,
        concurrentSessions,
        monthlyRevenue,
        openAlerts: openAlerts.length,
        criticalAlerts: openAlerts.filter(a => a.severity === 'CRITICAL').length,
        openTickets: openTickets.length,
        availableVouchers,
        activeAgents: db.agents.filter(a => a.status === 'ACTIVE').length,
        outboxBacklog,
        totalDownMbps: Number(db.zones.reduce((sum, z) => sum + z.currentDownMbps, 0).toFixed(1)),
        totalCapacityMbps: db.zones.reduce((sum, z) => sum + z.upstreamDownMbps, 0),
      },
      zones: db.zones,
      alerts: openAlerts.slice(0, 5),
      recentTransactions: db.transactions.slice(0, 6),
      bandwidthSeries: db.bandwidthSeries,
      revenueSeries: db.revenueSeries,
    })
  }

  // ── Admin: Zones & Zone Costs / P&L ────────────────────────────────────
  if (path === '/admin/zones' && method === 'GET') {
    return ok({
      zones: db.zones,
      costs: db.zoneCosts,
    })
  }

  if (path === '/admin/zones' && method === 'POST') {
    const body = await readBody(event)
    const zoneCode = String(body.zoneCode || '').trim().toUpperCase()
    const name = String(body.name || '').trim()
    if (!zoneCode || !name) {
      throw createError({ statusCode: 400, statusMessage: 'Zone code and zone name are required.' })
    }
    const created = {
      id: nextMockId('zone'),
      zoneCode,
      name,
      district: String(body.district || 'Accra Metro'),
      region: String(body.region || 'Greater Accra'),
      status: (body.status || 'PLANNED') as any,
      backhaulProvider: String(body.backhaulProvider || 'MTN_FIBRE'),
      backhaulCircuitRef: String(body.backhaulCircuitRef || `CIR-${zoneCode}-01`),
      upstreamDownMbps: Number(body.upstreamDownMbps || 100),
      upstreamUpMbps: Number(body.upstreamUpMbps || 50),
      upstreamMonthlyCost: Number(body.upstreamMonthlyCost || 1000),
      upstreamIsCgnat: Boolean(body.upstreamIsCgnat ?? true),
      tunnelIp: String(body.tunnelIp || `10.200.1.${50 + db.zones.length}`),
      gpsLat: Number(body.gpsLat || 5.56),
      gpsLng: Number(body.gpsLng || -0.21),
      onlineDevices: 0,
      totalDevices: 0,
      activeSessions: 0,
      activeSubscribers: 0,
      currentDownMbps: 0,
      currentUpMbps: 0,
      monthlyRevenue: 0,
      notes: String(body.notes || ''),
    }
    db.zones.unshift(created)
    db.auditLog.unshift({
      id: nextMockId('aud'),
      actorType: 'STAFF',
      actorName: 'Nana Kwame Admin',
      action: 'ZONE_CREATED',
      entity: 'magma_zones',
      entityId: created.id,
      summary: `Created zone ${created.name} (${created.zoneCode})`,
      ipAddress: '10.200.0.1',
      createdAt: new Date().toISOString(),
    })
    return ok(created, `Zone ${created.name} created.`)
  }

  const zoneUpdateMatch = path.match(/^\/admin\/zones\/([^/]+)$/)
  if (zoneUpdateMatch && method === 'PUT') {
    const zone = db.zones.find(z => z.id === zoneUpdateMatch[1])
    if (!zone) throw createError({ statusCode: 404, statusMessage: 'Zone not found.' })
    const body = await readBody(event)
    Object.assign(zone, {
      name: body.name ?? zone.name,
      district: body.district ?? zone.district,
      status: body.status ?? zone.status,
      backhaulProvider: body.backhaulProvider ?? zone.backhaulProvider,
      upstreamDownMbps: body.upstreamDownMbps !== undefined ? Number(body.upstreamDownMbps) : zone.upstreamDownMbps,
      upstreamUpMbps: body.upstreamUpMbps !== undefined ? Number(body.upstreamUpMbps) : zone.upstreamUpMbps,
      upstreamMonthlyCost: body.upstreamMonthlyCost !== undefined ? Number(body.upstreamMonthlyCost) : zone.upstreamMonthlyCost,
      tunnelIp: body.tunnelIp ?? zone.tunnelIp,
      notes: body.notes ?? zone.notes,
    })
    if (zone.status === 'CLOSED' || zone.status === 'MAINTENANCE') {
      zone.activeSessions = 0
      zone.currentDownMbps = 0
      zone.currentUpMbps = 0
    }
    return ok(zone, `Zone ${zone.name} updated to ${zone.status}.`)
  }

  const zoneCostMatch = path.match(/^\/admin\/zones\/([^/]+)\/costs$/)
  if (zoneCostMatch && method === 'POST') {
    const zone = db.zones.find(z => z.id === zoneCostMatch[1])
    if (!zone) throw createError({ statusCode: 404, statusMessage: 'Zone not found.' })
    const body = await readBody(event)
    const cost = {
      id: nextMockId('cost'),
      zoneId: zone.id,
      zoneName: zone.name,
      category: body.category || 'BACKHAUL',
      vendor: String(body.vendor || 'Vendor'),
      reference: String(body.reference || `COST-${Date.now().toString().slice(-4)}`),
      amount: Number(body.amount || 0),
      periodStart: String(body.periodStart || '2026-11-01'),
      periodEnd: String(body.periodEnd || '2026-11-30'),
      notes: String(body.notes || ''),
    }
    db.zoneCosts.unshift(cost)
    return ok(cost, 'Operating cost recorded for zone P&L.')
  }

  // ── Admin: Devices & RouterOS Fleet ────────────────────────────────────
  if (path === '/admin/devices' && method === 'GET') {
    const zoneId = String(query.zoneId || '')
    const list = zoneId ? db.devices.filter(d => d.zoneId === zoneId) : db.devices
    return ok(list)
  }

  if (path === '/admin/devices' && method === 'POST') {
    const body = await readBody(event)
    const zone = db.zones.find(z => z.id === body.zoneId) || db.zones[0]
    const created = {
      id: nextMockId('dev'),
      zoneId: zone.id,
      zoneName: zone.name,
      zoneCode: zone.zoneCode,
      name: String(body.name || 'New RouterOS Node'),
      role: (body.role || 'AP') as any,
      vendor: 'MikroTik',
      model: String(body.model || 'mANTBox ax 15s'),
      serialNumber: String(body.serialNumber || `MYN-SN-${Date.now().toString().slice(-5)}`),
      macAddress: String(body.macAddress || '48:8F:5A:24:09:99').toUpperCase(),
      routerosVersion: '7.16.2',
      managementIp: String(body.managementIp || '192.168.89.30'),
      tunnelIp: String(body.tunnelIp || '10.200.1.35'),
      restScheme: 'http' as const,
      restPort: 80,
      apiUsername: String(body.apiUsername || 'magma-api'),
      enrolmentStatus: 'ENROLLED' as const,
      status: 'ONLINE' as const,
      lastSeenAt: new Date().toISOString(),
      uptimeSeconds: 120,
      cpuLoad: 9,
      freeMemoryMb: 210,
      activeClients: 0,
      poeMode: String(body.poeMode || '24V Passive PoE'),
      notes: String(body.notes || ''),
    }
    db.devices.unshift(created)
    zone.totalDevices += 1
    zone.onlineDevices += 1
    return ok(created, `Device ${created.name} registered & enrolled in RADIUS NAS.`)
  }

  if (path === '/admin/devices/poll' && method === 'POST') {
    db.devices.forEach((d) => {
      if (d.status === 'ONLINE') {
        d.lastSeenAt = new Date().toISOString()
        d.uptimeSeconds += 60
        d.cpuLoad = Math.max(6, Math.min(68, d.cpuLoad + (Math.random() > 0.5 ? 2 : -2)))
      }
    })
    return ok({ polled: db.devices.length }, 'RouterOS REST fleet poll completed over WireGuard.')
  }

  const deviceActionMatch = path.match(/^\/admin\/devices\/([^/]+)\/(config|reboot|diagnostics)$/)
  if (deviceActionMatch) {
    const dev = db.devices.find(d => d.id === deviceActionMatch[1])
    if (!dev) throw createError({ statusCode: 404, statusMessage: 'Device not found.' })
    const action = deviceActionMatch[2]

    if (action === 'config' && method === 'POST') {
      dev.enrolmentStatus = 'ENROLLED'
      dev.status = 'ONLINE'
      dev.lastSeenAt = new Date().toISOString()
      return ok({
        deviceId: dev.id,
        steps: [
          { step: 'wireguard_peer', status: 'ok', detail: `Tunnel ${dev.tunnelIp} keepalive=25s` },
          { step: 'radius_client', status: 'ok', detail: 'Primary NAS 10.200.0.2 UDP/1812+1813, CoA 3799' },
          { step: 'walled_garden', status: 'ok', detail: 'Portal + Hubtel + Paystack TLS hosts permitted' },
          { step: 'hotspot_profile', status: 'ok', detail: 'MyNet SSID + 20MHz airtime fairness pushed' },
        ],
      }, `RouterOS configuration pushed to ${dev.name}.`)
    }

    if (action === 'reboot' && method === 'POST') {
      dev.uptimeSeconds = 15
      dev.lastSeenAt = new Date().toISOString()
      dev.status = 'ONLINE'
      return ok(dev, `Reboot command accepted by ${dev.name}.`)
    }

    if (action === 'diagnostics' && method === 'GET') {
      return ok({
        device: dev,
        pingMs: dev.status === 'ONLINE' ? 14.2 : null,
        wireguardHandshakeAgoSec: dev.status === 'ONLINE' ? 11 : null,
        coaPortOpen: dev.status === 'ONLINE',
        voltage: dev.poeMode.includes('24V') ? '24.2 V' : '26.8 V',
        temperatureC: dev.status === 'ONLINE' ? 43 : null,
        wlan2GhzChannel: '2412 MHz (Ch 1 · 20MHz)',
        wlan5GhzChannel: '5180 MHz (Ch 36 · 20MHz)',
      })
    }
  }

  // ── Admin: Data Plans & Catalogue ──────────────────────────────────────
  if (path === '/admin/plans' && method === 'GET') {
    return ok(db.plans)
  }

  if (path === '/admin/plans' && method === 'POST') {
    const body = await readBody(event)
    const planCode = String(body.planCode || '').trim().toUpperCase()
    if (!planCode || !body.name) {
      throw createError({ statusCode: 400, statusMessage: 'Plan code and name are required.' })
    }
    const price = Number(body.price || 10)
    const wholesalePrice = Number(body.wholesalePrice || (price * 0.85).toFixed(2))
    const plan = {
      id: nextMockId('plan'),
      planCode,
      name: String(body.name),
      planType: (body.planType || 'VOLUME') as any,
      durationMinutes: Number(body.durationMinutes || 1440),
      dataCapMb: Number(body.dataCapMb || 5120),
      fairUseCapMb: Number(body.fairUseCapMb || 0),
      downKbps: Number(body.downKbps || 15000),
      upKbps: Number(body.upKbps || 8000),
      throttleDownKbps: Number(body.throttleDownKbps || 0),
      throttleUpKbps: Number(body.throttleUpKbps || 0),
      burstDownKbps: Number(body.burstDownKbps || 20000),
      burstUpKbps: Number(body.burstUpKbps || 10000),
      deviceLimit: Number(body.deviceLimit || 1),
      price,
      wholesalePrice,
      agentCommission: Number((price - wholesalePrice).toFixed(2)),
      taxInclusive: Boolean(body.taxInclusive ?? true),
      validityDays: Number(body.validityDays || 7),
      isPublic: Boolean(body.isPublic ?? true),
      isActive: true,
      sortOrder: db.plans.length + 1,
      zoneIds: Array.isArray(body.zoneIds) && body.zoneIds.length ? body.zoneIds : db.zones.map(z => z.id),
      activeSubscriptions: 0,
      vouchersAvailable: 0,
    }
    db.plans.push(plan)
    return ok(plan, `Plan ${plan.name} added to MyNet catalogue.`)
  }

  const planUpdateMatch = path.match(/^\/admin\/plans\/([^/]+)$/)
  if (planUpdateMatch && method === 'PUT') {
    const plan = db.plans.find(p => p.id === planUpdateMatch[1])
    if (!plan) throw createError({ statusCode: 404, statusMessage: 'Plan not found.' })
    const body = await readBody(event)
    const price = body.price !== undefined ? Number(body.price) : plan.price
    const wholesalePrice = body.wholesalePrice !== undefined ? Number(body.wholesalePrice) : plan.wholesalePrice
    Object.assign(plan, {
      name: body.name ?? plan.name,
      planType: body.planType ?? plan.planType,
      durationMinutes: body.durationMinutes !== undefined ? Number(body.durationMinutes) : plan.durationMinutes,
      dataCapMb: body.dataCapMb !== undefined ? Number(body.dataCapMb) : plan.dataCapMb,
      downKbps: body.downKbps !== undefined ? Number(body.downKbps) : plan.downKbps,
      upKbps: body.upKbps !== undefined ? Number(body.upKbps) : plan.upKbps,
      deviceLimit: body.deviceLimit !== undefined ? Number(body.deviceLimit) : plan.deviceLimit,
      price,
      wholesalePrice,
      agentCommission: Number((price - wholesalePrice).toFixed(2)),
      isPublic: body.isPublic !== undefined ? Boolean(body.isPublic) : plan.isPublic,
      isActive: body.isActive !== undefined ? Boolean(body.isActive) : plan.isActive,
    })
    return ok(plan, `Plan ${plan.planCode} updated.`)
  }

  // ── Admin: Subscribers, RADIUS Sessions & Client MACs ──────────────────
  if (path === '/admin/subscribers' && method === 'GET') {
    return ok({
      subscribers: db.subscribers,
      sessions: db.sessions,
      clientDevices: db.clientDevices,
    })
  }

  if (path === '/admin/subscribers' && method === 'POST') {
    const body = await readBody(event)
    const zone = db.zones.find(z => z.id === body.homeZoneId) || db.zones[0]
    const plan = db.plans.find(p => p.planCode === body.planCode) || db.plans[1]
    const authType = (body.authType || (body.subscriberType === 'RESIDENTIAL' ? 'PPPOE' : 'HOTSPOT')) as 'HOTSPOT' | 'PPPOE'
    const created = {
      id: nextMockId('sub'),
      msisdn: String(body.msisdn || '').trim(),
      fullName: String(body.fullName || 'New MyNet Subscriber').trim(),
      email: String(body.email || ''),
      ghanaCardMasked: body.ghanaCardNumber ? `GHA-*****${String(body.ghanaCardNumber).slice(-4)}` : undefined,
      subscriberType: (body.subscriberType || 'WALK_IN') as any,
      authType,
      pppoeUsername: authType === 'PPPOE' ? `mg-pppoe-${Date.now().toString().slice(-3)}` : undefined,
      homeZoneId: zone.id,
      homeZoneName: zone.name,
      serviceAddress: String(body.serviceAddress || zone.name),
      status: 'ACTIVE' as const,
      kycStatus: body.ghanaCardNumber ? ('VERIFIED' as const) : ('NOT_REQUIRED' as const),
      activePlanCode: plan.planCode,
      activePlanName: plan.name,
      subscriptionId: nextMockId('subs'),
      subscriptionStatus: 'ACTIVE' as const,
      usedMb: 0,
      purchasedMb: plan.dataCapMb || 256000,
      expiresAt: new Date(Date.now() + plan.validityDays * 86400_000).toISOString(),
      onlineNow: false,
      deviceCount: 1,
      lifetimeSpend: plan.price,
      lastSeenAt: new Date().toISOString(),
      createdAt: new Date().toISOString(),
    }
    db.subscribers.unshift(created)
    return ok(created, `Subscriber ${created.fullName} onboarded & projected to FreeRADIUS.`)
  }

  const subActionMatch = path.match(/^\/admin\/subscribers\/([^/]+)\/(suspend|resume|extend|disconnect)$/)
  if (subActionMatch && method === 'POST') {
    const sub = db.subscribers.find(s => s.id === subActionMatch[1])
    if (!sub) throw createError({ statusCode: 404, statusMessage: 'Subscriber not found.' })
    const action = subActionMatch[2]
    if (action === 'suspend') {
      sub.status = 'SUSPENDED'
      sub.subscriptionStatus = 'SUSPENDED'
      sub.onlineNow = false
      db.sessions = db.sessions.filter(s => s.subscriberId !== sub.id)
      return ok(sub, `${sub.fullName} suspended & CoA Disconnect-Request sent.`)
    }
    if (action === 'resume') {
      sub.status = 'ACTIVE'
      sub.subscriptionStatus = 'ACTIVE'
      return ok(sub, `${sub.fullName} restored in radcheck.`)
    }
    if (action === 'extend') {
      const body = await readBody(event).catch(() => ({}))
      const extraDays = Number(body?.days || 3)
      const base = sub.expiresAt ? new Date(sub.expiresAt).getTime() : Date.now()
      sub.expiresAt = new Date(Math.max(Date.now(), base) + extraDays * 86400_000).toISOString()
      sub.status = 'ACTIVE'
      sub.subscriptionStatus = 'ACTIVE'
      return ok(sub, `Subscription extended by ${extraDays} day(s).`)
    }
    if (action === 'disconnect') {
      sub.onlineNow = false
      db.sessions = db.sessions.filter(s => s.subscriberId !== sub.id)
      return ok(sub, `Active RADIUS session terminated via CoA UDP/3799.`)
    }
  }

  const macToggleMatch = path.match(/^\/admin\/client-devices\/([^/]+)\/toggle-block$/)
  if (macToggleMatch && method === 'POST') {
    const cdev = db.clientDevices.find(d => d.id === macToggleMatch[1])
    if (!cdev) throw createError({ statusCode: 404, statusMessage: 'Client device not found.' })
    cdev.isBlocked = !cdev.isBlocked
    cdev.blockedReason = cdev.isBlocked ? 'Blocked by Admin Operator' : undefined
    return ok(cdev, cdev.isBlocked ? `MAC ${cdev.macAddress} blocked.` : `MAC ${cdev.macAddress} unblocked.`)
  }

  // ── Admin: Voucher Batches & Inventory ─────────────────────────────────
  if (path === '/admin/vouchers' && method === 'GET') {
    return ok({
      batches: db.voucherBatches,
      vouchers: db.vouchers,
    })
  }

  if (path === '/admin/vouchers/batches' && method === 'POST') {
    const body = await readBody(event)
    const plan = db.plans.find(p => p.id === body.planId || p.planCode === body.planCode) || db.plans[1]
    const zone = db.zones.find(z => z.id === body.zoneId) || db.zones[0]
    const qty = Math.max(1, Math.min(100, Number(body.quantity || 10)))
    const agent = body.ownerAgentId ? db.agents.find(a => a.id === body.ownerAgentId) : null

    const batchId = nextMockId('batch')
    const batchCode = `MYN-VB-${String(db.voucherBatches.length + 101)}`
    const expiresAt = new Date(Date.now() + Number(body.expiresInDays || 90) * 86400_000).toISOString()

    const batch = {
      id: batchId,
      batchCode,
      planId: plan.id,
      planCode: plan.planCode,
      planName: plan.name,
      zoneId: zone.id,
      zoneName: zone.name,
      quantity: qty,
      availableCount: agent ? 0 : qty,
      assignedCount: agent ? qty : 0,
      soldCount: 0,
      redeemedCount: 0,
      voidCount: 0,
      unitPrice: plan.price,
      wholesalePrice: plan.wholesalePrice,
      ownerType: (agent ? 'AGENT' : 'BUSINESS') as 'AGENT' | 'BUSINESS',
      ownerAgentId: agent?.id || null,
      ownerAgentName: agent?.name || null,
      status: 'ACTIVE' as const,
      expiresAt,
      createdAt: new Date().toISOString(),
    }
    db.voucherBatches.unshift(batch)

    const baseSeed = Date.now() % 10000
    for (let i = 1; i <= qty; i++) {
      const serialNum = baseSeed + i + db.vouchers.length
      db.vouchers.unshift({
        id: nextMockId('vch'),
        batchId: batch.id,
        batchCode: batch.batchCode,
        planId: plan.id,
        planCode: plan.planCode,
        planName: plan.name,
        allowanceLabel: `${plan.validityDays}d · ${plan.dataCapMb ? `${Math.round(plan.dataCapMb / 1024)} GB` : 'Unlimited'}`,
        zoneId: zone.id,
        zoneName: zone.name,
        zoneCode: zone.zoneCode,
        serial: `SN-${serialNum}`,
        code: generateVoucherCode(plan.planCode, serialNum),
        status: agent ? 'ASSIGNED' : 'AVAILABLE',
        agentId: agent?.id || null,
        agentName: agent?.name || null,
        price: plan.price,
        wholesalePrice: plan.wholesalePrice,
        commission: plan.agentCommission,
        expiresAt,
        createdAt: batch.createdAt,
      })
    }

    if (agent) recomputeAgentStock(db, agent.id)
    plan.vouchersAvailable += qty

    return ok(batch, `Issued ${qty} HMAC-protected vouchers in ${batch.batchCode}.`)
  }

  const batchActionMatch = path.match(/^\/admin\/vouchers\/batches\/([^/]+)\/(assign|void)$/)
  if (batchActionMatch && method === 'POST') {
    const batch = db.voucherBatches.find(b => b.id === batchActionMatch[1])
    if (!batch) throw createError({ statusCode: 404, statusMessage: 'Voucher batch not found.' })
    const action = batchActionMatch[2]

    if (action === 'assign') {
      const body = await readBody(event)
      const agent = db.agents.find(a => a.id === body.agentId)
      if (!agent) throw createError({ statusCode: 404, statusMessage: 'Target agent not found.' })
      batch.ownerType = 'AGENT'
      batch.ownerAgentId = agent.id
      batch.ownerAgentName = agent.name
      db.vouchers.forEach((v) => {
        if (v.batchId === batch.id && v.status === 'AVAILABLE') {
          v.status = 'ASSIGNED'
          v.agentId = agent.id
          v.agentName = agent.name
        }
      })
      recomputeBatchCounts(db, batch.id)
      recomputeAgentStock(db, agent.id)
      return ok(batch, `Assigned available stock in ${batch.batchCode} to ${agent.name}.`)
    }

    if (action === 'void') {
      batch.status = 'VOID'
      db.vouchers.forEach((v) => {
        if (v.batchId === batch.id && (v.status === 'AVAILABLE' || v.status === 'ASSIGNED')) {
          v.status = 'VOID'
        }
      })
      recomputeBatchCounts(db, batch.id)
      if (batch.ownerAgentId) recomputeAgentStock(db, batch.ownerAgentId)
      return ok(batch, `Unredeemed vouchers in ${batch.batchCode} voided.`)
    }
  }

  // ── Admin: Agents & Settlements ────────────────────────────────────────
  if (path === '/admin/agents' && method === 'GET') {
    return ok({
      agents: db.agents,
      ledger: db.agentLedger,
    })
  }

  if (path === '/admin/agents' && method === 'POST') {
    const body = await readBody(event)
    const zone = db.zones.find(z => z.id === body.territoryZoneId) || db.zones[0]
    const openingFloat = Number(body.openingFloat || 200)
    const created = {
      id: nextMockId('agent'),
      agentCode: `MYN-AG-00${db.agents.length + 1}`,
      name: String(body.name || 'New MyNet Agent'),
      msisdn: String(body.msisdn || ''),
      email: String(body.email || ''),
      tier: (body.tier || 'STANDARD') as any,
      commissionRate: Number(body.commissionRate || 0.12),
      territoryZoneId: zone.id,
      territoryZoneName: zone.name,
      territoryZoneCode: zone.zoneCode,
      walletAccountId: `TL-WAL-00${300 + db.agents.length}`,
      floatBalance: openingFloat,
      commissionEarnedMonth: 0,
      vouchersInStock: 0,
      vouchersSoldMonth: 0,
      status: 'ACTIVE' as const,
      kycStatus: 'VERIFIED' as const,
      createdAt: new Date().toISOString(),
    }
    db.agents.unshift(created)
    if (openingFloat > 0) {
      db.agentLedger.unshift({
        id: nextMockId('led'),
        agentId: created.id,
        agentName: created.name,
        entryType: 'ADJUSTMENT',
        amount: openingFloat,
        balanceAfter: openingFloat,
        reference: `MYN-OPEN-${created.agentCode}`,
        description: 'Opening float wallet credit',
        provider: 'WALLET',
        status: 'COMPLETED',
        createdAt: new Date().toISOString(),
      })
    }
    return ok(created, `Agent ${created.name} onboarded (${created.agentCode}).`)
  }

  const agentAdminAction = path.match(/^\/admin\/agents\/([^/]+)\/(topup|settle|toggle-status)$/)
  if (agentAdminAction && method === 'POST') {
    const agent = db.agents.find(a => a.id === agentAdminAction[1])
    if (!agent) throw createError({ statusCode: 404, statusMessage: 'Agent not found.' })
    const action = agentAdminAction[2]
    const body = await readBody(event).catch(() => ({}))

    if (action === 'toggle-status') {
      agent.status = agent.status === 'ACTIVE' ? 'SUSPENDED' : 'ACTIVE'
      return ok(agent, `Agent ${agent.name} is now ${agent.status}.`)
    }

    const amount = Number(body.amount || 0)
    if (amount <= 0) throw createError({ statusCode: 400, statusMessage: 'Enter a positive amount.' })

    if (action === 'topup') {
      agent.floatBalance = Number((agent.floatBalance + amount).toFixed(2))
      db.agentLedger.unshift({
        id: nextMockId('led'),
        agentId: agent.id,
        agentName: agent.name,
        entryType: 'FLOAT_TOPUP',
        amount,
        balanceAfter: agent.floatBalance,
        reference: `MYN-FLT-${Date.now().toString().slice(-4)}`,
        description: String(body.description || 'Admin float credit'),
        provider: String(body.provider || 'MOMO_MTN'),
        status: 'COMPLETED',
        createdAt: new Date().toISOString(),
      })
      return ok(agent, `Credited GH₵ ${amount.toFixed(2)} to ${agent.name}.`)
    }

    if (action === 'settle') {
      if (amount > agent.floatBalance) {
        throw createError({ statusCode: 400, statusMessage: 'Settlement exceeds agent wallet balance.' })
      }
      agent.floatBalance = Number((agent.floatBalance - amount).toFixed(2))
      db.agentLedger.unshift({
        id: nextMockId('led'),
        agentId: agent.id,
        agentName: agent.name,
        entryType: 'PAYOUT',
        amount: -amount,
        balanceAfter: agent.floatBalance,
        reference: `MYN-PAY-${Date.now().toString().slice(-4)}`,
        description: `MoMo payout settlement to ${agent.msisdn}`,
        provider: String(body.provider || 'MOMO_MTN'),
        status: 'COMPLETED',
        createdAt: new Date().toISOString(),
      })
      return ok(agent, `Disbursed GH₵ ${amount.toFixed(2)} MoMo payout to ${agent.name}.`)
    }
  }

  // ── Admin: Billing, Transactions, Invoices & Tax ───────────────────────
  if (path === '/admin/billing' && method === 'GET') {
    const successful = db.transactions.filter(t => t.status === 'SUCCESS')
    const gross = successful.reduce((s, t) => s + t.amount, 0)
    const net = successful.reduce((s, t) => s + t.netAmount, 0)
    const tax = successful.reduce((s, t) => s + t.taxAmount, 0)
    const vat = successful.reduce((s, t) => s + t.taxBreakdown.vat, 0)
    const nhil = successful.reduce((s, t) => s + t.taxBreakdown.nhil, 0)
    const getFund = successful.reduce((s, t) => s + t.taxBreakdown.getFund, 0)

    return ok({
      summary: {
        grossRevenue: Number(gross.toFixed(2)),
        netRevenue: Number(net.toFixed(2)),
        totalTax: Number(tax.toFixed(2)),
        vat: Number(vat.toFixed(2)),
        nhil: Number(nhil.toFixed(2)),
        getFund: Number(getFund.toFixed(2)),
        pendingCount: db.transactions.filter(t => t.status === 'PENDING').length,
        overdueInvoices: db.invoices.filter(i => i.status === 'OVERDUE').length,
      },
      transactions: db.transactions,
      invoices: db.invoices,
      revenueSeries: db.revenueSeries,
    })
  }

  const txActionMatch = path.match(/^\/admin\/transactions\/([^/]+)\/(verify|refund)$/)
  if (txActionMatch && method === 'POST') {
    const tx = db.transactions.find(t => t.id === txActionMatch[1])
    if (!tx) throw createError({ statusCode: 404, statusMessage: 'Transaction not found.' })
    if (txActionMatch[2] === 'verify') {
      tx.status = 'SUCCESS'
      tx.paidAt = new Date().toISOString()
      return ok(tx, `Transaction ${tx.internalReference} verified with ${tx.provider} & provisioned.`)
    }
    tx.status = 'REVERSED'
    return ok(tx, `Transaction ${tx.internalReference} refunded & subscription revoked.`)
  }

  if (path === '/admin/invoices/run' && method === 'POST') {
    const pppoeSubs = db.subscribers.filter(s => s.authType === 'PPPOE' && s.status === 'ACTIVE')
    const createdCount = pppoeSubs.length
    if (pppoeSubs[0]) {
      db.invoices.unshift({
        id: nextMockId('inv'),
        subscriberId: pppoeSubs[0].id,
        subscriberName: pppoeSubs[0].fullName,
        msisdn: pppoeSubs[0].msisdn,
        subscriptionId: pppoeSubs[0].subscriptionId || 'subs-new',
        planName: pppoeSubs[0].activePlanName || 'MyNet Home & Shop Monthly',
        zoneName: pppoeSubs[0].homeZoneName,
        invoiceNumber: `INV-MYN-00${45 + db.invoices.length}`,
        periodStart: '2026-12-01',
        periodEnd: '2026-12-31',
        subtotal: 100,
        taxAmount: 20,
        total: 120,
        amountPaid: 0,
        status: 'ISSUED',
        dueAt: new Date(Date.now() + 7 * 86400_000).toISOString(),
        issuedAt: new Date().toISOString(),
        paidAt: null,
      })
    }
    return ok({ generated: createdCount }, `Monthly PPPoE billing run issued ${createdCount} invoice(s).`)
  }

  // ── Admin: Monitoring, Alerts & Outbox ─────────────────────────────────
  if (path === '/admin/monitoring' && method === 'GET') {
    return ok({
      health: {
        api: 'HEALTHY',
        postgres: 'HEALTHY',
        redis: 'HEALTHY',
        freeradiusPrimary: 'HEALTHY',
        freeradiusStandby: 'HEALTHY',
        wireguardHub: 'HEALTHY',
        radiusAuthP95Ms: 84,
        coaDeliveryPct: 99.4,
      },
      alerts: db.alerts,
      outbox: db.outbox,
      bandwidthSeries: db.bandwidthSeries,
    })
  }

  const alertActionMatch = path.match(/^\/admin\/alerts\/([^/]+)\/(ack|resolve)$/)
  if (alertActionMatch && method === 'POST') {
    const alert = db.alerts.find(a => a.id === alertActionMatch[1])
    if (!alert) throw createError({ statusCode: 404, statusMessage: 'Alert not found.' })
    if (alertActionMatch[2] === 'ack') {
      alert.acknowledgedBy = 'Nana Kwame Admin'
      alert.acknowledgedAt = new Date().toISOString()
      return ok(alert, 'Alert acknowledged.')
    }
    alert.resolvedAt = new Date().toISOString()
    if (!alert.acknowledgedAt) {
      alert.acknowledgedBy = 'Nana Kwame Admin'
      alert.acknowledgedAt = alert.resolvedAt
    }
    return ok(alert, 'Alert marked resolved.')
  }

  if (path === '/admin/outbox/drain' && method === 'POST') {
    let processed = 0
    db.outbox.forEach((ev) => {
      if (ev.status !== 'DONE') {
        ev.status = 'DONE'
        ev.lastError = null
        ev.processedAt = new Date().toISOString()
        processed += 1
      }
    })
    return ok({ processed }, `Outbox worker drained ${processed} pending/dead-letter event(s).`)
  }

  // ── Admin: Support Tickets, Work Orders, Staff & Audit ─────────────────
  if (path === '/admin/support' && method === 'GET') {
    return ok({
      tickets: db.tickets,
      workOrders: db.workOrders,
      staff: db.staff,
    })
  }

  if (path === '/admin/tickets' && method === 'POST') {
    const body = await readBody(event)
    const zone = db.zones.find(z => z.id === body.zoneId) || db.zones[0]
    const tkt = {
      id: nextMockId('tkt'),
      ticketNumber: `TKT-${2044 + db.tickets.length}`,
      zoneId: zone.id,
      zoneName: zone.name,
      subscriberName: body.subscriberName || 'Walk-in Subscriber',
      subscriberMsisdn: body.subscriberMsisdn || '',
      category: (body.category || 'CONNECTIVITY') as any,
      priority: (body.priority || 'NORMAL') as any,
      status: 'OPEN' as const,
      subject: String(body.subject || 'Support issue'),
      description: String(body.description || ''),
      assignedTo: 'staff-support',
      assignedToName: 'Esi Helpdesk Lead',
      slaDueAt: new Date(Date.now() + 24 * 3600_000).toISOString(),
      createdAt: new Date().toISOString(),
      updatedAt: new Date().toISOString(),
      messages: [
        {
          id: nextMockId('tm'),
          ticketId: '',
          authorType: 'STAFF' as const,
          authorName: 'Helpdesk Intake',
          body: String(body.description || 'Ticket logged.'),
          createdAt: new Date().toISOString(),
        },
      ],
    }
    tkt.messages[0].ticketId = tkt.id
    db.tickets.unshift(tkt)
    return ok(tkt, `Ticket ${tkt.ticketNumber} opened.`)
  }

  const ticketReplyMatch = path.match(/^\/admin\/tickets\/([^/]+)\/reply$/)
  if (ticketReplyMatch && method === 'POST') {
    const tkt = db.tickets.find(t => t.id === ticketReplyMatch[1])
    if (!tkt) throw createError({ statusCode: 404, statusMessage: 'Ticket not found.' })
    const body = await readBody(event)
    if (body.message) {
      tkt.messages.push({
        id: nextMockId('tm'),
        ticketId: tkt.id,
        authorType: body.authorType || 'STAFF',
        authorName: body.authorName || 'Nana Kwame Admin',
        body: String(body.message),
        createdAt: new Date().toISOString(),
      })
    }
    if (body.status) {
      tkt.status = body.status
      if (body.status === 'RESOLVED' || body.status === 'CLOSED') {
        tkt.resolvedAt = new Date().toISOString()
      }
    }
    tkt.updatedAt = new Date().toISOString()
    return ok(tkt, `Ticket ${tkt.ticketNumber} updated.`)
  }

  if (path === '/admin/work-orders' && method === 'POST') {
    const body = await readBody(event)
    const zone = db.zones.find(z => z.id === body.zoneId) || db.zones[0]
    const staffMember = db.staff.find(s => s.id === body.assignedTo) || db.staff[2]
    const wo = {
      id: nextMockId('wo'),
      orderNumber: `WO-${1004 + db.workOrders.length}`,
      zoneId: zone.id,
      zoneName: zone.name,
      subscriberName: body.subscriberName || null,
      serviceAddress: String(body.serviceAddress || zone.name),
      orderType: (body.orderType || 'INSTALL') as any,
      status: 'SCHEDULED' as const,
      assignedTo: staffMember.id,
      assignedToName: staffMember.fullName,
      scheduledAt: body.scheduledAt || new Date(Date.now() + 86400_000).toISOString(),
      checklist: Array.isArray(body.checklist) && body.checklist.length
        ? body.checklist
        : ['Site safety & grounding check', 'Verify 24V passive PoE voltage', 'Confirm subscriber throughput'],
      materials: Array.isArray(body.materials) ? body.materials : [],
      cost: Number(body.cost || 0),
      completedAt: null,
      createdAt: new Date().toISOString(),
    }
    db.workOrders.unshift(wo)
    return ok(wo, `Work order ${wo.orderNumber} scheduled.`)
  }

  const woStatusMatch = path.match(/^\/admin\/work-orders\/([^/]+)\/status$/)
  if (woStatusMatch && method === 'POST') {
    const wo = db.workOrders.find(w => w.id === woStatusMatch[1])
    if (!wo) throw createError({ statusCode: 404, statusMessage: 'Work order not found.' })
    const body = await readBody(event)
    wo.status = body.status || 'DONE'
    if (wo.status === 'DONE') wo.completedAt = new Date().toISOString()
    return ok(wo, `${wo.orderNumber} marked ${wo.status}.`)
  }

  if (path === '/admin/system' && method === 'GET') {
    return ok({
      business: db.business,
      staff: db.staff,
      auditLog: db.auditLog,
    })
  }

  if (path === '/admin/staff' && method === 'POST') {
    const body = await readBody(event)
    const member = {
      id: nextMockId('staff'),
      fullName: String(body.fullName || 'New Staff'),
      msisdn: String(body.msisdn || ''),
      email: String(body.email || ''),
      role: (body.role || 'SUPPORT') as any,
      zoneIds: db.zones.map(z => z.id),
      zoneNames: ['All Zones'],
      status: 'ACTIVE' as const,
      lastLoginAt: new Date().toISOString(),
      createdAt: new Date().toISOString(),
    }
    db.staff.push(member)
    return ok(member, `${member.fullName} added to MyNet staff directory.`)
  }

  // ══════════════════════════════════════════════════════════════════════
  // AGENT PORTAL ENDPOINTS (`/api/mynet/agent/*`)
  // ══════════════════════════════════════════════════════════════════════

  if (path === '/agent/dashboard' && method === 'GET') {
    const agentId = getActiveAgentId(event, String(query.agentId || 'agent-1'))
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    recomputeAgentStock(db, agent.id)

    const agentVouchers = db.vouchers.filter(v => v.agentId === agent.id)
    const soldVouchers = agentVouchers.filter(v => v.status === 'SOLD' || v.status === 'REDEEMED')
    const stockByPlan = db.plans.map((p) => {
      const count = agentVouchers.filter(v => v.planId === p.id && v.status === 'ASSIGNED').length
      return {
        planId: p.id,
        planCode: p.planCode,
        planName: p.name,
        price: p.price,
        wholesalePrice: p.wholesalePrice,
        commission: Number((p.price * agent.commissionRate).toFixed(2)),
        inStock: count,
      }
    })

    const territoryZone = db.zones.find(z => z.id === agent.territoryZoneId) || db.zones[0]
    const ledger = db.agentLedger.filter(l => l.agentId === agent.id)

    return ok({
      agent,
      territoryZone,
      stockByPlan,
      recentSales: soldVouchers.slice(0, 8),
      recentLedger: ledger.slice(0, 6),
      dailySalesChart: [
        { day: 'Mon', sales: 120, commission: 18 },
        { day: 'Tue', sales: 168, commission: 25.2 },
        { day: 'Wed', sales: 144, commission: 21.6 },
        { day: 'Thu', sales: 216, commission: 32.4 },
        { day: 'Fri', sales: 264, commission: 39.6 },
        { day: 'Sat', sales: 312, commission: 46.8 },
        { day: 'Sun', sales: 196, commission: 29.4 },
      ],
    })
  }

  if (path === '/agent/catalog' && method === 'GET') {
    const agentId = getActiveAgentId(event)
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    const plans = db.plans.filter(p => p.isActive && p.isPublic).map((p) => {
      const inStock = db.vouchers.filter(v => v.agentId === agent.id && v.planId === p.id && v.status === 'ASSIGNED').length
      const commission = Number((p.price * agent.commissionRate).toFixed(2))
      const netDebit = Number((p.price - commission).toFixed(2))
      return {
        ...p,
        inStock,
        effectiveCommission: commission,
        netFloatCost: netDebit,
      }
    })
    return ok({
      agent,
      zones: db.zones.filter(z => z.status === 'LIVE'),
      plans,
    })
  }

  if (path === '/agent/sell' && method === 'POST') {
    const agentId = getActiveAgentId(event)
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    const body = await readBody(event)

    const plan = db.plans.find(p => p.id === body.planId || p.planCode === body.planCode)
    if (!plan) throw createError({ statusCode: 404, statusMessage: 'Selected data bundle not found.' })

    const zone = db.zones.find(z => z.id === (body.zoneId || agent.territoryZoneId)) || db.zones[0]
    const source = body.source === 'float' ? 'float' : 'stock'
    const customerMsisdn = String(body.customerMsisdn || '').trim()
    const customerName = String(body.customerName || 'Walk-in Customer').trim()

    // Try selling from pre-assigned stock first if available & requested
    const stockedVoucher = db.vouchers.find(
      v => v.agentId === agent.id && v.planId === plan.id && v.status === 'ASSIGNED',
    )

    const commission = Number((plan.price * agent.commissionRate).toFixed(2))
    const netCost = Number((plan.price - commission).toFixed(2))

    let soldVoucher = stockedVoucher

    if (source === 'stock' && stockedVoucher) {
      stockedVoucher.status = 'SOLD'
      stockedVoucher.customerMsisdn = customerMsisdn || null
      stockedVoucher.customerName = customerName
      stockedVoucher.soldAt = new Date().toISOString()
      recomputeBatchCounts(db, stockedVoucher.batchId)
      recomputeAgentStock(db, agent.id)
      agent.vouchersSoldMonth += 1
      agent.commissionEarnedMonth = Number((agent.commissionEarnedMonth + stockedVoucher.commission).toFixed(2))
    } else {
      // Instant PIN generation from agent float wallet
      if (agent.floatBalance < netCost) {
        throw createError({
          statusCode: 400,
          statusMessage: `Insufficient float balance (need GH₵ ${netCost.toFixed(2)}, have GH₵ ${agent.floatBalance.toFixed(2)}). Top up your float first.`,
        })
      }
      agent.floatBalance = Number((agent.floatBalance - netCost).toFixed(2))
      agent.vouchersSoldMonth += 1
      agent.commissionEarnedMonth = Number((agent.commissionEarnedMonth + commission).toFixed(2))

      const serialSeed = Date.now() % 100000
      soldVoucher = {
        id: nextMockId('vch'),
        batchId: 'instant-pos',
        batchCode: 'INSTANT-POS',
        planId: plan.id,
        planCode: plan.planCode,
        planName: plan.name,
        allowanceLabel: `${plan.validityDays}d · ${plan.dataCapMb ? `${Math.round(plan.dataCapMb / 1024)} GB` : 'Unlimited'}`,
        zoneId: zone.id,
        zoneName: zone.name,
        zoneCode: zone.zoneCode,
        serial: `POS-${serialSeed}`,
        code: generateVoucherCode(plan.planCode, serialSeed),
        status: 'SOLD',
        agentId: agent.id,
        agentName: agent.name,
        price: plan.price,
        wholesalePrice: netCost,
        commission,
        customerMsisdn: customerMsisdn || null,
        customerName,
        soldAt: new Date().toISOString(),
        expiresAt: new Date(Date.now() + 60 * 86400_000).toISOString(),
        createdAt: new Date().toISOString(),
      }
      db.vouchers.unshift(soldVoucher)

      db.agentLedger.unshift({
        id: nextMockId('led'),
        agentId: agent.id,
        agentName: agent.name,
        entryType: 'VOUCHER_SALE',
        amount: -netCost,
        balanceAfter: agent.floatBalance,
        reference: soldVoucher.serial,
        description: `Instant PIN ${plan.planCode} sold (${customerMsisdn || 'Walk-in'}) · Earned GH₵ ${commission.toFixed(2)}`,
        provider: 'WALLET',
        status: 'COMPLETED',
        createdAt: new Date().toISOString(),
      })
    }

    const netAmt = Number((plan.price / 1.2).toFixed(2))
    db.transactions.unshift({
      id: nextMockId('tx'),
      subscriberName: customerName,
      payerMsisdn: customerMsisdn || agent.msisdn,
      agentId: agent.id,
      agentName: agent.name,
      voucherId: soldVoucher!.id,
      zoneId: zone.id,
      zoneName: zone.name,
      planCode: plan.planCode,
      planName: plan.name,
      purpose: 'VOUCHER',
      amount: plan.price,
      netAmount: netAmt,
      taxAmount: Number((plan.price - netAmt).toFixed(2)),
      taxBreakdown: {
        vat: Number((netAmt * 0.15).toFixed(2)),
        nhil: Number((netAmt * 0.025).toFixed(2)),
        getFund: Number((netAmt * 0.025).toFixed(2)),
      },
      paymentMethod: 'CASH_FLOAT',
      provider: 'WALLET',
      providerReference: soldVoucher!.serial,
      internalReference: `MYN-POS-${soldVoucher!.serial}`,
      status: 'SUCCESS',
      initiatedAt: new Date().toISOString(),
      paidAt: new Date().toISOString(),
    })

    return ok({
      voucher: soldVoucher,
      agent,
      portalRedeemUrl: `https://wifi.mynet.gh/?zone=${encodeURIComponent(zone.zoneCode)}&voucher=${encodeURIComponent(soldVoucher!.code)}`,
    }, `Voucher ${soldVoucher!.code} issued!`)
  }

  if (path === '/agent/stock' && method === 'GET') {
    const agentId = getActiveAgentId(event)
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    recomputeAgentStock(db, agent.id)
    return ok({
      agent,
      vouchers: db.vouchers.filter(v => v.agentId === agent.id),
      batches: db.voucherBatches.filter(b => b.ownerAgentId === agent.id),
      plans: db.plans.filter(p => p.isActive),
      zones: db.zones.filter(z => z.status === 'LIVE'),
    })
  }

  if (path === '/agent/stock/purchase' && method === 'POST') {
    const agentId = getActiveAgentId(event)
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    const body = await readBody(event)
    const plan = db.plans.find(p => p.id === body.planId || p.planCode === body.planCode)
    if (!plan) throw createError({ statusCode: 404, statusMessage: 'Plan not found.' })
    const qty = Math.max(1, Math.min(100, Number(body.quantity || 10)))
    const zone = db.zones.find(z => z.id === (body.zoneId || agent.territoryZoneId)) || db.zones[0]

    const unitCost = Number((plan.price * (1 - agent.commissionRate)).toFixed(2))
    const totalCost = Number((unitCost * qty).toFixed(2))
    if (agent.floatBalance < totalCost) {
      throw createError({
        statusCode: 400,
        statusMessage: `Float balance GH₵ ${agent.floatBalance.toFixed(2)} is below wholesale cost GH₵ ${totalCost.toFixed(2)}.`,
      })
    }

    agent.floatBalance = Number((agent.floatBalance - totalCost).toFixed(2))

    const batchId = nextMockId('batch')
    const batchCode = `MYN-AGB-${db.voucherBatches.length + 201}`
    const expiresAt = new Date(Date.now() + 90 * 86400_000).toISOString()

    const batch = {
      id: batchId,
      batchCode,
      planId: plan.id,
      planCode: plan.planCode,
      planName: plan.name,
      zoneId: zone.id,
      zoneName: zone.name,
      quantity: qty,
      availableCount: 0,
      assignedCount: qty,
      soldCount: 0,
      redeemedCount: 0,
      voidCount: 0,
      unitPrice: plan.price,
      wholesalePrice: unitCost,
      ownerType: 'AGENT' as const,
      ownerAgentId: agent.id,
      ownerAgentName: agent.name,
      status: 'ACTIVE' as const,
      expiresAt,
      createdAt: new Date().toISOString(),
    }
    db.voucherBatches.unshift(batch)

    const seed = Date.now() % 10000
    for (let i = 1; i <= qty; i++) {
      const n = seed + i + db.vouchers.length
      db.vouchers.unshift({
        id: nextMockId('vch'),
        batchId: batch.id,
        batchCode: batch.batchCode,
        planId: plan.id,
        planCode: plan.planCode,
        planName: plan.name,
        allowanceLabel: `${plan.validityDays}d · ${plan.dataCapMb ? `${Math.round(plan.dataCapMb / 1024)} GB` : 'Unlimited'}`,
        zoneId: zone.id,
        zoneName: zone.name,
        zoneCode: zone.zoneCode,
        serial: `SN-${n}`,
        code: generateVoucherCode(plan.planCode, n),
        status: 'ASSIGNED',
        agentId: agent.id,
        agentName: agent.name,
        price: plan.price,
        wholesalePrice: unitCost,
        commission: Number((plan.price - unitCost).toFixed(2)),
        expiresAt,
        createdAt: batch.createdAt,
      })
    }

    recomputeAgentStock(db, agent.id)
    db.agentLedger.unshift({
      id: nextMockId('led'),
      agentId: agent.id,
      agentName: agent.name,
      entryType: 'STOCK_PURCHASE',
      amount: -totalCost,
      balanceAfter: agent.floatBalance,
      reference: batchCode,
      description: `Wholesale bundle stock: ${qty} × ${plan.planCode}`,
      provider: 'WALLET',
      status: 'COMPLETED',
      createdAt: new Date().toISOString(),
    })

    return ok({ batch, agent }, `Purchased ${qty} × ${plan.planCode} vouchers into your stock!`)
  }

  if (path === '/agent/wallet' && method === 'GET') {
    const agentId = getActiveAgentId(event)
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    return ok({
      agent,
      ledger: db.agentLedger.filter(l => l.agentId === agent.id),
    })
  }

  if (path === '/agent/wallet/topup' && method === 'POST') {
    const agentId = getActiveAgentId(event)
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    const body = await readBody(event)
    const amount = Number(body.amount || 0)
    if (amount < 10) {
      throw createError({ statusCode: 400, statusMessage: 'Minimum float top-up is GH₵ 10.00.' })
    }
    agent.floatBalance = Number((agent.floatBalance + amount).toFixed(2))
    const entry = {
      id: nextMockId('led'),
      agentId: agent.id,
      agentName: agent.name,
      entryType: 'FLOAT_TOPUP' as const,
      amount,
      balanceAfter: agent.floatBalance,
      reference: `MYN-MOMO-${Date.now().toString().slice(-5)}`,
      description: `${body.provider || 'MOMO_MTN'} float top-up from ${body.msisdn || agent.msisdn}`,
      provider: String(body.provider || 'MOMO_MTN'),
      status: 'COMPLETED' as const,
      createdAt: new Date().toISOString(),
    }
    db.agentLedger.unshift(entry)
    return ok({ agent, entry }, `Float wallet topped up by GH₵ ${amount.toFixed(2)}.`)
  }

  if (path === '/agent/wallet/settlement' && method === 'POST') {
    const agentId = getActiveAgentId(event)
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    const body = await readBody(event)
    const amount = Number(body.amount || 0)
    if (amount <= 0 || amount > agent.floatBalance) {
      throw createError({ statusCode: 400, statusMessage: 'Enter a valid payout amount within your wallet balance.' })
    }
    agent.floatBalance = Number((agent.floatBalance - amount).toFixed(2))
    const entry = {
      id: nextMockId('led'),
      agentId: agent.id,
      agentName: agent.name,
      entryType: 'PAYOUT' as const,
      amount: -amount,
      balanceAfter: agent.floatBalance,
      reference: `MYN-SET-${Date.now().toString().slice(-5)}`,
      description: `MoMo settlement payout to ${body.msisdn || agent.msisdn} (${body.provider || 'MOMO_MTN'})`,
      provider: String(body.provider || 'MOMO_MTN'),
      status: 'COMPLETED' as const,
      createdAt: new Date().toISOString(),
    }
    db.agentLedger.unshift(entry)
    return ok({ agent, entry }, `Payout of GH₵ ${amount.toFixed(2)} sent to ${body.msisdn || agent.msisdn}.`)
  }

  if (path === '/agent/subscriber-lookup' && method === 'GET') {
    const q = String(query.q || '').trim().toLowerCase()
    const matches = q
      ? db.subscribers.filter(s => s.msisdn.includes(q) || s.fullName.toLowerCase().includes(q))
      : db.subscribers.slice(0, 5)
    return ok({
      subscribers: matches,
      plans: db.plans.filter(p => p.isActive && p.isPublic),
    })
  }

  if (path === '/agent/subscriber-topup' && method === 'POST') {
    const agentId = getActiveAgentId(event)
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    const body = await readBody(event)
    const plan = db.plans.find(p => p.id === body.planId || p.planCode === body.planCode)
    if (!plan) throw createError({ statusCode: 404, statusMessage: 'Plan not found.' })

    const msisdn = String(body.msisdn || '').trim()
    if (!msisdn) throw createError({ statusCode: 400, statusMessage: 'Subscriber phone number is required.' })

    const commission = Number((plan.price * agent.commissionRate).toFixed(2))
    const netDebit = Number((plan.price - commission).toFixed(2))
    if (agent.floatBalance < netDebit) {
      throw createError({ statusCode: 400, statusMessage: 'Insufficient float balance for direct top-up.' })
    }

    let sub = db.subscribers.find(s => s.msisdn === msisdn)
    if (!sub) {
      sub = {
        id: nextMockId('sub'),
        msisdn,
        fullName: String(body.fullName || `Walk-in (${msisdn})`),
        email: '',
        subscriberType: 'WALK_IN',
        authType: 'HOTSPOT',
        homeZoneId: agent.territoryZoneId,
        homeZoneName: agent.territoryZoneName,
        serviceAddress: agent.territoryZoneName,
        status: 'ACTIVE',
        kycStatus: 'NOT_REQUIRED',
        usedMb: 0,
        purchasedMb: plan.dataCapMb || 256000,
        onlineNow: true,
        deviceCount: 1,
        lifetimeSpend: 0,
        lastSeenAt: new Date().toISOString(),
        createdAt: new Date().toISOString(),
      }
      db.subscribers.unshift(sub)
    }

    sub.status = 'ACTIVE'
    sub.subscriptionStatus = 'ACTIVE'
    sub.activePlanCode = plan.planCode
    sub.activePlanName = plan.name
    sub.purchasedMb = plan.dataCapMb || 256000
    sub.usedMb = 0
    sub.expiresAt = new Date(Date.now() + plan.validityDays * 86400_000).toISOString()
    sub.lifetimeSpend += plan.price

    agent.floatBalance = Number((agent.floatBalance - netDebit).toFixed(2))
    agent.commissionEarnedMonth = Number((agent.commissionEarnedMonth + commission).toFixed(2))

    db.agentLedger.unshift({
      id: nextMockId('led'),
      agentId: agent.id,
      agentName: agent.name,
      entryType: 'VOUCHER_SALE',
      amount: -netDebit,
      balanceAfter: agent.floatBalance,
      reference: `TOPUP-${Date.now().toString().slice(-4)}`,
      description: `Direct bundle activation ${plan.planCode} for ${sub.fullName} (${msisdn})`,
      provider: 'WALLET',
      status: 'COMPLETED',
      createdAt: new Date().toISOString(),
    })

    return ok({ subscriber: sub, agent, commission }, `Activated ${plan.name} for ${sub.fullName} (${msisdn}).`)
  }

  if (path === '/agent/tickets' && method === 'GET') {
    const agentId = getActiveAgentId(event)
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    return ok({
      agent,
      zones: db.zones,
      tickets: db.tickets.filter(t => t.agentId === agent.id || t.zoneId === agent.territoryZoneId),
    })
  }

  if (path === '/agent/tickets' && method === 'POST') {
    const agentId = getActiveAgentId(event)
    const agent = db.agents.find(a => a.id === agentId) || db.agents[0]
    const body = await readBody(event)
    const zone = db.zones.find(z => z.id === (body.zoneId || agent.territoryZoneId)) || db.zones[0]
    const tkt = {
      id: nextMockId('tkt'),
      ticketNumber: `TKT-${2045 + db.tickets.length}`,
      zoneId: zone.id,
      zoneName: zone.name,
      agentId: agent.id,
      agentName: agent.name,
      subscriberMsisdn: body.subscriberMsisdn || null,
      category: (body.category || 'CONNECTIVITY') as any,
      priority: (body.priority || 'HIGH') as any,
      status: 'OPEN' as const,
      subject: String(body.subject || 'Field issue reported by agent'),
      description: String(body.description || ''),
      assignedTo: 'staff-support',
      assignedToName: 'Esi Helpdesk Lead',
      slaDueAt: new Date(Date.now() + 12 * 3600_000).toISOString(),
      createdAt: new Date().toISOString(),
      updatedAt: new Date().toISOString(),
      messages: [
        {
          id: nextMockId('tm'),
          ticketId: '',
          authorType: 'AGENT' as const,
          authorName: agent.name,
          body: String(body.description || body.subject),
          createdAt: new Date().toISOString(),
        },
      ],
    }
    tkt.messages[0].ticketId = tkt.id
    db.tickets.unshift(tkt)
    return ok(tkt, `Field issue ${tkt.ticketNumber} escalated to MyNet NOC.`)
  }

  throw createError({
    statusCode: 404,
    statusMessage: `Unknown mock MyNet endpoint: ${method} ${path}`,
  })
})
