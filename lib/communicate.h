#ifndef _COMMUNICATE_H
#define _COMMUNICATE_H

#include<time.h>

#include "commons.h"

typedef enum Action {
	Login,
	Transaction,
	BalanceEnquiry,
	PasswordChange,
	ViewDetails,
	Logout
} Action;

typedef enum Status {
	Success,
	Unauthorized,
	Failure,
	InternalError,
	NoFunds
} Status;

typedef struct Header {
	Action action;
	id_t sid;
} Header;

typedef struct LoginRequest {
    char uname[INFO_STRING_LEN];
    char password[INFO_STRING_LEN];
} LoginRequest;

typedef struct LoginResponse {
    id_t sid;
    Status stat;
    int is_admin;
} LoginResponse;

typedef struct TransactionRequest {
    balance_t amt;
} TransactionRequest;

typedef struct TransactionResponse {
    Status stat;
    id_t id;
    balance_t old_balance;
    balance_t new_balance;
} TransactionResponse;

typedef struct BalanceEnquiryResponse {
    Status stat;
    balance_t balance;
} BalanceEnquiryResponse;

typedef struct PasswordChangeRequest {
    char password[INFO_STRING_LEN];
} PasswordChangeRequest;

typedef struct PasswordChangeResponse {
    Status stat;
} PasswordChangeResponse;

typedef struct LogoutResponse {
    Status stat;
} LogoutResponse;

typedef struct ViewDetailsResponse {
    Status stat;
    id_t uid, aid;
    balance_t balance;
    char name[INFO_STRING_LEN];
    char uname[INFO_STRING_LEN];
    char password[INFO_STRING_LEN];
    int is_admin;
    int num_trans;
} ViewDetailsResponse;

typedef struct TransactionItem {
    Status stat;
    id_t uid;
    id_t id;
    time_t when;
    balance_t old_balance;
    balance_t new_balance;
} TransactionItem;
#endif
