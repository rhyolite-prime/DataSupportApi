/**
 * Ghana Cedi (GH₵ / GHS), bandwidth, data volume, duration & timestamp
 * formatting helpers shared across MyNet Admin and MyNet Agent portals.
 */

export function formatCurrency(
  value: number | string | null | undefined,
  currency = 'GHS',
  options: Intl.NumberFormatOptions = {},
): string {
  const num = typeof value === 'string' ? Number(value) : value
  if (num === null || num === undefined || !Number.isFinite(num)) return '—'
  return new Intl.NumberFormat('en-GH', {
    style: 'currency',
    currency,
    minimumFractionDigits: 2,
    maximumFractionDigits: 2,
    ...options,
  }).format(num)
}

export function formatCompact(value: number | string | null | undefined): string {
  const num = typeof value === 'string' ? Number(value) : value
  if (num === null || num === undefined || !Number.isFinite(num)) return '—'
  return new Intl.NumberFormat('en-GH', {
    notation: 'compact',
    maximumFractionDigits: 1,
  }).format(num)
}

export function formatDateTime(value: string | Date | null | undefined): string {
  if (!value) return '—'
  const date = value instanceof Date ? value : new Date(value)
  if (Number.isNaN(date.getTime())) return '—'
  return new Intl.DateTimeFormat('en-GH', {
    day: '2-digit',
    month: 'short',
    year: 'numeric',
    hour: '2-digit',
    minute: '2-digit',
  }).format(date)
}

export function formatDate(value: string | Date | null | undefined): string {
  if (!value) return '—'
  const date = value instanceof Date ? value : new Date(value)
  if (Number.isNaN(date.getTime())) return '—'
  return new Intl.DateTimeFormat('en-GH', {
    day: '2-digit',
    month: 'short',
    year: 'numeric',
  }).format(date)
}

export function formatRelativeTime(value: string | Date | null | undefined): string {
  if (!value) return 'Never'
  const date = value instanceof Date ? value : new Date(value)
  if (Number.isNaN(date.getTime())) return '—'
  const diffSec = Math.round((Date.now() - date.getTime()) / 1000)
  if (diffSec < 60) return `${Math.max(1, diffSec)}s ago`
  if (diffSec < 3600) return `${Math.floor(diffSec / 60)}m ago`
  if (diffSec < 86400) return `${Math.floor(diffSec / 3600)}h ago`
  return `${Math.floor(diffSec / 86400)}d ago`
}

export function formatDataVolume(mb: number | null | undefined): string {
  if (mb === null || mb === undefined || mb <= 0) return 'Unlimited'
  if (mb >= 1024 * 1024) return `${(mb / (1024 * 1024)).toFixed(1)} TB`
  if (mb >= 1024) {
    const gb = mb / 1024
    return `${Number.isInteger(gb) ? gb : gb.toFixed(1)} GB`
  }
  return `${Math.round(mb)} MB`
}

export function formatOctets(octets: number | null | undefined): string {
  if (!octets || octets <= 0) return '0 MB'
  const mb = octets / (1024 * 1024)
  return formatDataVolume(mb)
}

export function formatSpeedKbps(kbps: number | null | undefined): string {
  if (!kbps || kbps <= 0) return '—'
  if (kbps >= 1000) {
    const mbps = kbps / 1000
    return `${Number.isInteger(mbps) ? mbps : mbps.toFixed(1)} Mbps`
  }
  return `${kbps} kbps`
}

export function formatDurationMinutes(minutes: number | null | undefined): string {
  if (!minutes || minutes <= 0) return '30 days'
  if (minutes < 60) return `${minutes} min`
  if (minutes < 1440) {
    const hrs = minutes / 60
    return `${Number.isInteger(hrs) ? hrs : hrs.toFixed(1)} hr${hrs === 1 ? '' : 's'}`
  }
  const days = Math.round(minutes / 1440)
  return `${days} day${days === 1 ? '' : 's'}`
}

export function formatUptime(seconds: number | null | undefined): string {
  if (!seconds || seconds <= 0) return 'Offline'
  const days = Math.floor(seconds / 86400)
  const hours = Math.floor((seconds % 86400) / 3600)
  const mins = Math.floor((seconds % 3600) / 60)
  if (days > 0) return `${days}d ${hours}h`
  if (hours > 0) return `${hours}h ${mins}m`
  return `${mins}m`
}

export function initialsFromName(name: string | null | undefined): string {
  if (!name) return 'MN'
  return name
    .split(/\s+/)
    .filter(Boolean)
    .slice(0, 2)
    .map(part => part.charAt(0).toUpperCase())
    .join('') || 'MN'
}
