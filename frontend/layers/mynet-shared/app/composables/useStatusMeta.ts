import type { StatusTone } from '../types/models'

const STATUS_MAP: Record<string, { label: string; tone: StatusTone }> = {
  // Zones & devices
  LIVE: { label: 'Live', tone: 'green' },
  ONLINE: { label: 'Online', tone: 'green' },
  ENROLLED: { label: 'Enrolled', tone: 'green' },
  BUILD: { label: 'In Build', tone: 'amber' },
  PLANNED: { label: 'Planned', tone: 'blue' },
  MAINTENANCE: { label: 'Maintenance', tone: 'amber' },
  DEGRADED: { label: 'Degraded', tone: 'amber' },
  OFFLINE: { label: 'Offline', tone: 'red' },
  UNKNOWN: { label: 'Pending Link', tone: 'gray' },
  CLOSED: { label: 'Closed', tone: 'gray' },
  DECOMMISSIONED: { label: 'Decommissioned', tone: 'gray' },

  // Subscribers & subscriptions
  ACTIVE: { label: 'Active', tone: 'green' },
  SUSPENDED: { label: 'Suspended', tone: 'amber' },
  CHURNED: { label: 'Churned', tone: 'red' },
  EXPIRED: { label: 'Expired', tone: 'gray' },
  DEPLETED: { label: 'Depleted', tone: 'amber' },
  CANCELLED: { label: 'Cancelled', tone: 'red' },

  // KYC & tiers
  VERIFIED: { label: 'Verified', tone: 'green' },
  NOT_REQUIRED: { label: 'Walk-in', tone: 'gray' },
  PENDING: { label: 'Pending', tone: 'amber' },
  STANDARD: { label: 'Standard', tone: 'blue' },
  PREMIER: { label: 'Premier', tone: 'purple' },
  SUPER: { label: 'Super Agent', tone: 'green' },

  // Vouchers
  AVAILABLE: { label: 'Available', tone: 'green' },
  ASSIGNED: { label: 'In Stock', tone: 'blue' },
  SOLD: { label: 'Sold', tone: 'purple' },
  REDEEMED: { label: 'Redeemed', tone: 'green' },
  VOID: { label: 'Voided', tone: 'red' },

  // Payments & invoices
  SUCCESS: { label: 'Paid', tone: 'green' },
  COMPLETED: { label: 'Completed', tone: 'green' },
  PAID: { label: 'Paid', tone: 'green' },
  ISSUED: { label: 'Issued', tone: 'blue' },
  OVERDUE: { label: 'Overdue', tone: 'red' },
  DRAFT: { label: 'Draft', tone: 'gray' },
  FAILED: { label: 'Failed', tone: 'red' },
  REVERSED: { label: 'Refunded', tone: 'purple' },

  // Alerts, tickets & work orders
  CRITICAL: { label: 'Critical', tone: 'red' },
  URGENT: { label: 'Urgent', tone: 'red' },
  HIGH: { label: 'High', tone: 'amber' },
  WARNING: { label: 'Warning', tone: 'amber' },
  NORMAL: { label: 'Normal', tone: 'blue' },
  LOW: { label: 'Low', tone: 'gray' },
  INFO: { label: 'Info', tone: 'blue' },
  OPEN: { label: 'Open', tone: 'red' },
  IN_PROGRESS: { label: 'In Progress', tone: 'amber' },
  RESOLVED: { label: 'Resolved', tone: 'green' },
  SCHEDULED: { label: 'Scheduled', tone: 'blue' },
  EN_ROUTE: { label: 'En Route', tone: 'amber' },
  DONE: { label: 'Completed', tone: 'green' },
  DEAD: { label: 'Dead Letter', tone: 'red' },
}

export function useStatusMeta() {
  function statusMeta(code: string | null | undefined): { label: string; tone: StatusTone } {
    if (!code) return { label: '—', tone: 'gray' }
    const normalized = code.toUpperCase()
    if (STATUS_MAP[normalized]) return STATUS_MAP[normalized]
    return {
      label: code.replace(/_/g, ' ').toLowerCase().replace(/\b\w/g, c => c.toUpperCase()),
      tone: 'gray',
    }
  }

  return { statusMeta }
}
