import type { PortalNavigationGroup, PortalSearchItem } from '@mynet-shared/types/models'

export const adminNavigationGroups: PortalNavigationGroup[] = [
  {
    title: 'Network & Access',
    icon: 'wifi',
    links: [
      { label: 'Zones & Sites', to: '/zones', icon: 'map-pin' },
      { label: 'RouterOS Fleet', to: '/devices', icon: 'server-stack' },
      { label: 'Internet Plans', to: '/plans', icon: 'bolt' },
      { label: 'Monitoring & Outbox', to: '/monitoring', icon: 'signal' },
    ],
  },
  {
    title: 'Customers & Sales',
    icon: 'users',
    links: [
      { label: 'Subscribers & Sessions', to: '/subscribers', icon: 'user-group' },
      { label: 'Voucher Batches', to: '/vouchers', icon: 'ticket' },
      { label: 'Reseller Agents', to: '/agents', icon: 'wallet' },
    ],
  },
  {
    title: 'Finance & Desk',
    icon: 'banknotes',
    links: [
      { label: 'Billing, Revenue & Tax', to: '/billing', icon: 'banknotes' },
      { label: 'Support & Work Orders', to: '/support', icon: 'wrench-screwdriver' },
      { label: 'Staff, Keys & Audit', to: '/system', icon: 'shield-check' },
    ],
  },
]

export const adminSearchItems: PortalSearchItem[] = [
  { label: 'NOC & Commercial Overview', detail: 'Live concurrent sessions, backhaul Mbps, zone health & revenue', to: '/', icon: 'home' },
  { label: 'Zones & Coverage Sites', detail: 'Open/close hotspot zones, WireGuard tunnel IPs, backhaul & zone P&L', to: '/zones', icon: 'map-pin' },
  { label: 'MikroTik RouterOS Fleet', detail: 'RB5009 gateways, mANTBox ax 15s sectors, push config, CoA & diagnostics', to: '/devices', icon: 'server-stack' },
  { label: 'Internet Data Plans', detail: 'Prepaid hotspot & residential PPPoE plans, FUP speeds & agent commissions', to: '/plans', icon: 'bolt' },
  { label: 'Subscribers & Live Sessions', detail: 'MSISDN lookup, PPPoE onboarding, suspend/resume, CoA disconnect & MAC block', to: '/subscribers', icon: 'user-group' },
  { label: 'Voucher Batches & Stock', detail: 'Issue HMAC voucher batches, assign to field agents, export CSV & void stock', to: '/vouchers', icon: 'ticket' },
  { label: 'Reseller Agents & Wallets', detail: 'Agent tiers, TurboLedger float wallets, top-ups & MoMo settlements', to: '/agents', icon: 'wallet' },
  { label: 'Billing, Revenue & Ghana Tax', detail: 'Hubtel MoMo, Paystack card, PPPoE invoices, VAT/NHIL/GETFund breakdown', to: '/billing', icon: 'banknotes' },
  { label: 'Monitoring, Alerts & Outbox', detail: 'Acknowledge/resolve NOC alerts, FreeRADIUS SLA, drain outbox queue', to: '/monitoring', icon: 'signal' },
  { label: 'Support Desk & Field Work Orders', detail: 'Customer/agent tickets, CPE installs & 24V PoE field dispatches', to: '/support', icon: 'wrench-screwdriver' },
  { label: 'Staff Roles, API Keys & Audit', detail: 'Staff PIN operators, DataSupportApi credentials & immutable audit trail', to: '/system', icon: 'shield-check' },
]

export function adminPageTitle(path: string): string {
  const found = adminSearchItems.find(i => (i.to === '/' ? path === '/' : path.startsWith(i.to)))
  return found?.label || 'Network Control Plane'
}
