# MyNet Broadband Frontend Portals (Nuxt 4 + Tailwind CSS v4 + Nuxt Layers)

Commercial UI portals for **MyNet Broadband** (the customer- and reseller-facing brand powered by the `MagmaNetService` / `DataSupportApi` C++ control plane).

## Architecture

```text
frontend/
├── layers/
│   └── mynet-shared/          # Shared Nuxt 4 Layer (@mynet/shared-layer)
│       ├── app/
│       │   ├── assets/        # Tailwind v4 @theme tokens + self-hosted Sora font
│       │   ├── components/    # 22 shared UI & portal shell components
│       │   ├── composables/   # useBrand, useTheme, useToast, useStatusMeta, useMyNetApi, usePortalSession
│       │   ├── types/         # 1:1 TypeScript models matching MagmaNet DTOs
│       │   └── utils/         # Ghana GHS/telemetry formatters & SVG icon registry
│       └── server/
│           ├── api/mynet/     # Same-origin Nitro API router (/api/mynet/*)
│           └── utils/         # Stateful in-memory MyNet Ghana mock database
└── apps/
    ├── mynet-admin/           # Standalone Nuxt 4 project — Admin & NOC Control Plane (:3100)
    └── mynet-agent/           # Standalone Nuxt 4 project — Reseller & Field Agent POS (:3101)
```

Each application in `apps/*` extends `../../layers/mynet-shared` and produces its own completely independent `.output/` production bundle (`node .output/server/index.mjs`), so **MyNet Admin** and **MyNet Agent** can be deployed onto separate hosts, containers, or subdomains (`admin.mynet.gh` and `agent.mynet.gh`).

## Visual & Color System

Follows the workspace layout, typography (`Sora` + tabular numerals), card hierarchy, split auth shell, and operational attention patterns from `BusinessDesk`, with a distinct telecommunications palette tailored for **MyNet**:
- **MyNet Admin (`mynet-cobalt`)**: Deep Sapphire `#1E40AF`, Electric Cyan `#06B6D4` accent, Midnight Navy `#0B132B` sidebar.
- **MyNet Agent (`mynet-indigo-coral`)**: Royal Indigo `#4338CA`, Energetic Coral `#F97316` POS accent, Deep Indigo `#13112E` sidebar.
- Full `html.dark` dark mode + live white-label palette switcher in the top bar.

## Development & Standalone Production Builds

```bash
cd frontend
npm install

# Run dev servers
npm run dev:admin     # http://0.0.0.0:3100
npm run dev:agent     # http://0.0.0.0:3101

# Build each portal as an independent deployable Nuxt output
npm run build:admin   # outputs apps/mynet-admin/.output
npm run build:agent   # outputs apps/mynet-agent/.output
```

## Plugging In the Live C++ Backend (`DataSupportApi`)

All Vue pages call same-origin `/api/mynet/*` via `useMyNetApi()`. Currently, the shared layer's Nitro handler serves stateful mock data so every workflow (minting voucher batches, pushing RouterOS configs, CoA session disconnects, offline POS queue sync, QR slip printing, MoMo float top-ups, and GRA VAT/NHIL/GETFund schedules) works interactively out of the box.

When ready to connect to the live Drogon `DataSupportApi` backend (`:5702`), set:

```bash
NUXT_MAGMA_API_BASE_URL=https://api.mynet.gh
```

and forward requests server-side inside `layers/mynet-shared/server/api/mynet/[...slug].ts` without modifying any page components.
