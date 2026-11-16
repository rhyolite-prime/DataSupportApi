/**
 * Domain types shared between MyNet Admin Portal, MyNet Agent Portal,
 * and the Nitro mock / DataSupportApi bridge layer.
 * Mirrors the MagmaNet control-plane schema & DTOs in DataSupportApi.
 */

export type StatusTone = 'green' | 'amber' | 'red' | 'blue' | 'gray' | 'purple'

export interface PortalNavigationLink {
  label: string
  to?: string
  icon: string
  badge?: string | number
  comingSoon?: boolean
  children?: PortalNavigationLink[]
}

export interface PortalNavigationGroup {
  title: string
  icon: string
  links: PortalNavigationLink[]
}

export interface PortalSearchItem {
  label: string
  detail: string
  to: string
  icon?: string
}

export interface PortalUserSession {
  authenticated: boolean
  portal: 'admin' | 'agent'
  id: string
  name: string
  role: string
  msisdn: string
  email?: string
  code?: string
  zoneIds?: string[]
  territoryZoneId?: string
  territoryZoneName?: string
  tier?: string
  commissionRate?: number
  tenantName: string
  businessId: string
}

export type ZoneStatus = 'PLANNED' | 'BUILD' | 'LIVE' | 'MAINTENANCE' | 'CLOSED'

export interface MyNetZone {
  id: string
  zoneCode: string
  name: string
  district: string
  region: string
  status: ZoneStatus
  backhaulProvider: string
  backhaulCircuitRef: string
  upstreamDownMbps: number
  upstreamUpMbps: number
  upstreamMonthlyCost: number
  upstreamIsCgnat: boolean
  tunnelIp: string
  publicIp?: string
  gpsLat: number
  gpsLng: number
  onlineDevices: number
  totalDevices: number
  activeSessions: number
  activeSubscribers: number
  currentDownMbps: number
  currentUpMbps: number
  monthlyRevenue: number
  notes: string
}

export interface MyNetZoneCost {
  id: string
  zoneId: string
  zoneName: string
  category: 'BACKHAUL' | 'POWER' | 'RENT' | 'MAINTENANCE' | 'SECURITY' | 'OTHER'
  vendor: string
  reference: string
  amount: number
  periodStart: string
  periodEnd: string
  notes: string
}

export type PlanType = 'TIME' | 'VOLUME' | 'UNLIMITED' | 'HYBRID'

export interface MyNetDataPlan {
  id: string
  planCode: string
  name: string
  planType: PlanType
  durationMinutes: number
  dataCapMb: number
  fairUseCapMb: number
  downKbps: number
  upKbps: number
  throttleDownKbps: number
  throttleUpKbps: number
  burstDownKbps: number
  burstUpKbps: number
  deviceLimit: number
  price: number
  wholesalePrice: number
  agentCommission: number
  taxInclusive: boolean
  validityDays: number
  isPublic: boolean
  isActive: boolean
  sortOrder: number
  zoneIds: string[]
  activeSubscriptions: number
  vouchersAvailable: number
}

export interface MyNetDevice {
  id: string
  zoneId: string
  zoneName: string
  zoneCode: string
  name: string
  role: 'GATEWAY' | 'AP' | 'SWITCH' | 'CPE'
  vendor: string
  model: string
  serialNumber: string
  macAddress: string
  routerosVersion: string
  managementIp: string
  tunnelIp: string
  restScheme: 'http' | 'https'
  restPort: number
  apiUsername: string
  enrolmentStatus: 'PENDING' | 'ENROLLED' | 'DECOMMISSIONED'
  status: 'ONLINE' | 'OFFLINE' | 'DEGRADED' | 'UNKNOWN'
  lastSeenAt: string | null
  uptimeSeconds: number
  cpuLoad: number
  freeMemoryMb: number
  activeClients: number
  poeMode: string
  notes: string
}

export interface MyNetSubscriber {
  id: string
  msisdn: string
  fullName: string
  email: string
  ghanaCardMasked?: string
  subscriberType: 'WALK_IN' | 'RESIDENTIAL' | 'BUSINESS'
  authType: 'HOTSPOT' | 'PPPOE'
  pppoeUsername?: string
  homeZoneId: string
  homeZoneName: string
  serviceAddress: string
  status: 'ACTIVE' | 'SUSPENDED' | 'CHURNED'
  kycStatus: 'VERIFIED' | 'PENDING' | 'NOT_REQUIRED'
  activePlanCode?: string
  activePlanName?: string
  subscriptionId?: string
  subscriptionStatus?: 'ACTIVE' | 'SUSPENDED' | 'EXPIRED' | 'DEPLETED' | 'CANCELLED'
  usedMb: number
  purchasedMb: number
  expiresAt?: string
  onlineNow: boolean
  deviceCount: number
  lifetimeSpend: number
  lastSeenAt: string
  createdAt: string
}

export interface MyNetClientDevice {
  id: string
  subscriberId: string
  macAddress: string
  label: string
  firstSeenAt: string
  lastSeenAt: string
  isBlocked: boolean
  blockedReason?: string
}

export interface MyNetSession {
  id: string
  subscriptionId: string
  subscriberId: string
  subscriberName: string
  msisdn: string
  zoneId: string
  zoneName: string
  deviceId: string
  deviceName: string
  acctSessionId: string
  nasIp: string
  framedIp: string
  macAddress: string
  startedAt: string
  lastInterimAt: string
  endedAt?: string | null
  inputOctets: number
  outputOctets: number
  sessionSeconds: number
  terminateCause?: string | null
}

export interface MyNetVoucherBatch {
  id: string
  batchCode: string
  planId: string
  planCode: string
  planName: string
  zoneId: string
  zoneName: string
  quantity: number
  availableCount: number
  assignedCount: number
  soldCount: number
  redeemedCount: number
  voidCount: number
  unitPrice: number
  wholesalePrice: number
  ownerType: 'BUSINESS' | 'AGENT'
  ownerAgentId?: string | null
  ownerAgentName?: string | null
  status: 'ACTIVE' | 'DEPLETED' | 'VOID'
  expiresAt: string
  createdAt: string
}

export interface MyNetVoucher {
  id: string
  batchId: string
  batchCode: string
  planId: string
  planCode: string
  planName: string
  allowanceLabel: string
  zoneId: string
  zoneName: string
  zoneCode: string
  serial: string
  code: string
  status: 'AVAILABLE' | 'ASSIGNED' | 'SOLD' | 'REDEEMED' | 'VOID' | 'EXPIRED'
  agentId?: string | null
  agentName?: string | null
  price: number
  wholesalePrice: number
  commission: number
  customerMsisdn?: string | null
  customerName?: string | null
  soldAt?: string | null
  redeemedAt?: string | null
  expiresAt: string
  createdAt: string
}

export interface MyNetAgent {
  id: string
  agentCode: string
  name: string
  msisdn: string
  email: string
  tier: 'STANDARD' | 'PREMIER' | 'SUPER'
  commissionRate: number
  territoryZoneId: string
  territoryZoneName: string
  territoryZoneCode: string
  walletAccountId: string
  floatBalance: number
  commissionEarnedMonth: number
  vouchersInStock: number
  vouchersSoldMonth: number
  status: 'ACTIVE' | 'SUSPENDED'
  kycStatus: 'VERIFIED' | 'PENDING'
  createdAt: string
}

export interface MyNetAgentLedgerEntry {
  id: string
  agentId: string
  agentName: string
  entryType: 'ADJUSTMENT' | 'FLOAT_TOPUP' | 'STOCK_PURCHASE' | 'VOUCHER_SALE' | 'EARNING' | 'PAYOUT'
  amount: number
  balanceAfter: number
  reference: string
  description: string
  provider?: string
  status: 'COMPLETED' | 'PENDING'
  createdAt: string
}

export interface MyNetTransaction {
  id: string
  subscriberId?: string
  subscriberName: string
  payerMsisdn: string
  subscriptionId?: string
  agentId?: string | null
  agentName?: string | null
  voucherId?: string | null
  zoneId: string
  zoneName: string
  planCode: string
  planName: string
  purpose: 'BUNDLE' | 'VOUCHER' | 'INVOICE' | 'FLOAT_TOPUP'
  amount: number
  taxAmount: number
  netAmount: number
  taxBreakdown: { vat: number; nhil: number; getFund: number }
  paymentMethod: 'MOMO_MTN' | 'MOMO_TELECEL' | 'MOMO_AIRTELTIGO' | 'CARD' | 'CASH_FLOAT'
  provider: 'HUBTEL' | 'PAYSTACK' | 'WALLET'
  providerReference: string
  internalReference: string
  status: 'SUCCESS' | 'PENDING' | 'FAILED' | 'REVERSED'
  initiatedAt: string
  paidAt?: string | null
}

export interface MyNetInvoice {
  id: string
  subscriberId: string
  subscriberName: string
  msisdn: string
  subscriptionId: string
  planName: string
  zoneName: string
  invoiceNumber: string
  periodStart: string
  periodEnd: string
  subtotal: number
  taxAmount: number
  total: number
  amountPaid: number
  status: 'PAID' | 'ISSUED' | 'OVERDUE' | 'DRAFT'
  dueAt: string
  issuedAt: string
  paidAt?: string | null
}

export interface MyNetAlert {
  id: string
  zoneId?: string | null
  zoneName?: string | null
  deviceId?: string | null
  deviceName?: string | null
  alertType: string
  severity: 'CRITICAL' | 'WARNING' | 'INFO'
  message: string
  acknowledgedBy?: string | null
  acknowledgedAt?: string | null
  resolvedAt?: string | null
  createdAt: string
}

export interface MyNetTicketMessage {
  id: string
  ticketId: string
  authorType: 'CUSTOMER' | 'AGENT' | 'STAFF'
  authorName: string
  body: string
  createdAt: string
}

export interface MyNetTicket {
  id: string
  ticketNumber: string
  zoneId: string
  zoneName: string
  subscriberId?: string | null
  subscriberName?: string | null
  subscriberMsisdn?: string | null
  agentId?: string | null
  agentName?: string | null
  category: 'CONNECTIVITY' | 'BILLING' | 'VOUCHER' | 'DEVICE' | 'INSTALL' | 'OTHER'
  priority: 'URGENT' | 'HIGH' | 'NORMAL' | 'LOW'
  status: 'OPEN' | 'IN_PROGRESS' | 'RESOLVED' | 'CLOSED'
  subject: string
  description: string
  assignedTo?: string | null
  assignedToName?: string | null
  slaDueAt: string
  resolvedAt?: string | null
  createdAt: string
  updatedAt: string
  messages: MyNetTicketMessage[]
}

export interface MyNetWorkOrder {
  id: string
  orderNumber: string
  zoneId: string
  zoneName: string
  subscriberId?: string | null
  subscriberName?: string | null
  serviceAddress: string
  orderType: 'INSTALL' | 'REPAIR' | 'SURVEY' | 'RECOVERY'
  status: 'SCHEDULED' | 'EN_ROUTE' | 'IN_PROGRESS' | 'DONE' | 'CANCELLED'
  assignedTo: string
  assignedToName: string
  scheduledAt: string
  checklist: string[]
  materials: string[]
  cost: number
  completedAt?: string | null
  createdAt: string
}

export interface MyNetStaffMember {
  id: string
  fullName: string
  msisdn: string
  email: string
  role: 'ADMIN' | 'OPS' | 'TECHNICIAN' | 'SUPPORT' | 'FINANCE'
  zoneIds: string[]
  zoneNames: string[]
  status: 'ACTIVE' | 'SUSPENDED'
  lastLoginAt: string
  createdAt: string
}

export interface MyNetOutboxEvent {
  id: string
  eventType: 'RADIUS_PROVISION' | 'RADIUS_DEPROVISION' | 'COA_DISCONNECT' | 'COA_RATE_LIMIT' | 'SMS_SEND' | 'SUBSCRIPTION_ENDED'
  aggregateId: string
  summary: string
  status: 'PENDING' | 'DONE' | 'DEAD'
  attempts: number
  nextAttemptAt: string
  lastError?: string | null
  createdAt: string
  processedAt?: string | null
}

export interface MyNetAuditEntry {
  id: string
  actorType: 'STAFF' | 'AGENT' | 'SYSTEM'
  actorName: string
  action: string
  entity: string
  entityId: string
  summary: string
  ipAddress: string
  createdAt: string
}
