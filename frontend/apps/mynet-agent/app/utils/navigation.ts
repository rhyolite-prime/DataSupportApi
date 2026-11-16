import type { NavGroup } from '@mynet-shared/components/PortalSidebar.vue'
import type { QuickSearchItem } from '@mynet-shared/components/PortalTopbar.vue'

export const agentNavigationGroups: NavGroup[] = [
  {
    label: 'Point of Sale',
    items: [
      { label: 'Sell Instant Voucher', to: '/sell', icon: 'bolt', badge: 'POS' },
      { label: 'Direct Subscriber Top-Up', to: '/topup', icon: 'device-phone-mobile' },
    ],
  },
  {
    label: 'Inventory & Earnings',
    items: [
      { label: 'Voucher Stock & Batches', to: '/stock', icon: 'ticket' },
      { label: 'Float Wallet & MoMo Payout', to: '/wallet', icon: 'wallet' },
    ],
  },
  {
    label: 'Territory Support',
    items: [
      { label: 'Escalate Issue & Profile', to: '/support', icon: 'lifebuoy' },
    ],
  },
]

export const agentSearchItems: QuickSearchItem[] = [
  { label: 'Reseller Dashboard', sub: 'Today sales, float balance & commission performance', to: '/', icon: 'squares-2x2' },
  { label: 'Instant Voucher POS', sub: 'Issue WiFi PIN, SMS or print QR voucher slip', to: '/sell', icon: 'bolt' },
  { label: 'Direct Subscriber Top-Up', sub: 'Search customer MSISDN & activate internet bundle', to: '/topup', icon: 'device-phone-mobile' },
  { label: 'Voucher Stock & Wholesale Batches', sub: 'Buy discounted bundle stock into kiosk inventory', to: '/stock', icon: 'ticket' },
  { label: 'Float Wallet & MoMo Settlement', sub: 'Top up float via MTN/Telecel or cash out commissions', to: '/wallet', icon: 'wallet' },
  { label: 'NOC Field Support & Profile', sub: 'Report WiFi zone outage or verify reseller tier', to: '/support', icon: 'lifebuoy' },
]

export function agentPageTitle(path: string): string {
  const map: Record<string, string> = {
    '/': 'Reseller Overview',
    '/sell': 'Instant Voucher POS',
    '/topup': 'Direct Subscriber Top-Up',
    '/stock': 'Voucher Stock & Wholesale',
    '/wallet': 'Float Wallet & Commissions',
    '/support': 'Support & Reseller Profile',
  }
  return map[path] || 'MyNet Agent POS'
}
