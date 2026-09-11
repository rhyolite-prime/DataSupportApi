# WAEC Evoucher Service Rebuild

This document outlines the proposed database schemas and implementation plan for the WAEC Evoucher Service. The service caters to two main actors: Individuals (retail) and Merchants (wholesale/affiliate).

## User Review Required

> [!IMPORTANT]
> Please review the proposed database schema and the plan for integrating with **TurboLedger**. Specifically, let me know if TurboLedger runs as a separate microservice with its own database, or if I should create local ledger tables in `DataSupportDb` for this project.


## API Controllers

> [!NOTE]
> I will implement the methods outlined in `WaecVoucherService.h` and also create standard Drogon REST controllers for these endpoints so they can be accessed over HTTP.

## Hashing for Vouchers and Merchant PINs

- The merchant `pin_hash` will be securely hashed using Bcrypt.
- The `pin` in `waec_vouchers` will also be hashed. This implies that once a voucher is generated/imported and sold, the plain text PIN must be delivered to the buyer immediately (e.g., in the response, via SMS, or Email), as it cannot be recovered from the database afterward.

## TurboLedger Integration (Apache Fineract Approach)

Since **TurboLedger** is an external microservice modeled after Apache Fineract, the most suitable approach for managing merchant commissions is to use the **Client** and **Savings Account** modules (often used for digital wallets).

**Proposed Fineract Module Mapping:**
1. **Client Module (`/clients`)**: Every time a Merchant account is created in `DataSupportApi`, we create a corresponding Client in TurboLedger.
2. **Savings Products Module (`/savingsproducts`)**: You will need a predefined Savings Product (e.g., "Merchant Commission Wallet") in TurboLedger.
3. **Savings Account Module (`/savingsaccounts`)**: 
   - We open a Savings Account for the Merchant Client using the Commission Wallet product. The resulting `accountId` is stored as the `commission_account_id` in our `waec_merchants` table.
   - **Earning Commission**: When an affiliate sale happens, we hit the Savings Account Deposit API (`/savingsaccounts/{accountId}/transactions?command=deposit`) to add funds.
   - **Selling from Wallet / Cashing out**: When the merchant buys vouchers using their commission, we hit the Savings Account Withdrawal API (`/savingsaccounts/{accountId}/transactions?command=withdrawal`).
   - **Transaction History**: To generate the merchant statement, we simply fetch the Savings Account transactions (`/savingsaccounts/{accountId}/transactions`).

This approach gives us double-entry accounting for free while providing out-of-the-box wallet capabilities like balances, transaction history, and status management.

## Proposed Changes

### Database Schema

We will create the following tables in the PostgreSQL database (`DataSupportDb`). After creating them, we will use `drogon_ctl` to generate the corresponding C++ models.

```sql
-- 1. Merchants Table
CREATE TABLE waec_merchants (
    id VARCHAR(36) PRIMARY KEY,
    business_id VARCHAR(36) REFERENCES business_accounts(id),
    merchant_name VARCHAR(255),
    phone_number VARCHAR(20) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE,
    pin_hash VARCHAR(255) NOT NULL,
    is_affiliate BOOLEAN DEFAULT FALSE,
    status VARCHAR(50) DEFAULT 'ACTIVE',
    commission_account_id VARCHAR(36), -- Link to TurboLedger account
    main_account_id VARCHAR(36),       -- Link to TurboLedger main account (for bulk buyers)
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 2. Vouchers Table
CREATE TABLE waec_vouchers (
    id VARCHAR(36) PRIMARY KEY,
    batch_id VARCHAR(100),
    serial_number VARCHAR(100) UNIQUE NOT NULL,
    pin VARCHAR(255) NOT NULL,
    status VARCHAR(50) DEFAULT 'AVAILABLE', -- AVAILABLE, SOLD, USED
    price DECIMAL(10, 2) NOT NULL,
    wholesale_price DECIMAL(10, 2),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP
);

-- 3. Voucher Sales Table (Transactions)
CREATE TABLE waec_voucher_sales (
    id VARCHAR(36) PRIMARY KEY,
    voucher_id VARCHAR(36) REFERENCES waec_vouchers(id),
    merchant_id VARCHAR(36) REFERENCES waec_merchants(id), -- Null if direct retail
    buyer_phone VARCHAR(20),
    buyer_email VARCHAR(255),
    sale_type VARCHAR(50), -- RETAIL, BULK, AFFILIATE
    amount_paid DECIMAL(10, 2) NOT NULL,
    commission_earned DECIMAL(10, 2) DEFAULT 0,
    payment_reference VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

*(Note: The Ledger accounts and transactions will be entirely managed via API calls to TurboLedger, so no local ledger tables are needed in this database.)*

---

### Data Models & DTOs

#### [NEW] `dto/waec_voucher/WaecMerchantAuthDto.h`
- Create a base DTO class containing `merchantId` and `pin` (plain text pin sent by client). All merchant operation DTOs will inherit from this to avoid repetition.

#### [NEW] `dto/waec_voucher/WaecVoucherDto.h` (Update)
- Update DTOs based on the new schema and commented out methods in `WaecVoucherService.h` (`IndividualWaecVoucherSaleDto`, `BulkWaecVoucherSaleDto`, `AffiliateWaecVoucherSaleDto`, `WaecVoucherMerchantDto`, etc.). Make sure relevant merchant DTOs inherit from `WaecMerchantAuthDto`.

---

### Services

#### [MODIFY] `services/waec_evoucher/WaecVoucherService.h`
- Uncomment and refine existing methods.
- **Add Extra Methods**:
  - `getMerchantDashboardStats`: Retrieves total sales, total commission earned, and current commission balance.
  - `getMerchantTransactions`: Paginated history of sales made by the merchant.
  - `getMerchantStatement`: Retrieves the merchant's ledger statement (via TurboLedger integration).
  - `cashOutCommission`: Allows merchants to move commission to a Momo wallet or use it as their main balance.
  - `getAvailableVoucherCount`: Helper to check current stock before making a bulk purchase.

#### [MODIFY] `services/waec_evoucher/WaecVoucherService.cc`
- Implement business logic for each API endpoint.
- Incorporate Ledger logic for commissions (calculating commission, updating merchant wallet).
- Handle bulk buying logic vs affiliate USSD proxy selling logic.
- Implement PIN reset and merchant info update functionality.

---

### Controllers

#### [NEW] `controllers/WaecVoucherController.h` / `.cc`
- Create REST endpoints that hook into `WaecVoucherService`.

## Verification Plan

### Automated Tests
- If there are unit tests present, we will write C++ tests using Drogon's testing framework to ensure endpoints return the correct responses.

### Manual Verification
- We will manually review the generated models to ensure DB mappings are correct.
- We will execute sample requests to test merchant creation, bulk purchase, affiliate purchase, and ledger updates.
