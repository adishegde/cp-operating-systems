#ifndef _COMMUNICATE_H
#define _COMMUNICATE_H

#include <time.h>

#include "commons.h"

typedef enum Action {
    Login,
    Transaction,
    BalanceEnquiry,
    PasswordChange,
    ViewDetails,
    Logout,
    CreateUser,
    ModifyUser,
    GetUsers,
    DeleteUser
} Action;

typedef enum Status {
    Success,
    Unauthorized,
    Failure,
    InternalError,
    NoFunds,
    Conflict,
    NotFound
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

typedef struct CreateUserRequest {
    char name[INFO_STRING_LEN];
    char uname[INFO_STRING_LEN];
    char password[INFO_STRING_LEN];
    int is_admin;
    id_t aid; // 0 if creating new account
} CreateUserRequest;

typedef struct CreateUserResponse {
    Status stat;
    id_t uid;
    id_t aid;
} CreateUserResponse;

typedef struct GetUsersResponse {
    Status stat;
    int num_users;
} GetUsersResponse;

typedef struct UserItem {
    Status stat;
    char name[INFO_STRING_LEN];
    char uname[INFO_STRING_LEN];
    char password[INFO_STRING_LEN];
    int is_admin;
    id_t aid;
    id_t id;
} UserItem;

typedef struct DeleteUserRequest {
    id_t uid;
} DeleteUserRequest;

typedef struct DeleteUserResponse {
    Status stat;
} DeleteUserResponse;

typedef struct ModifyUserRequest {
    char name[INFO_STRING_LEN];
    char password[INFO_STRING_LEN];
    int is_admin;
    id_t uid;
} ModifyUserRequest;

typedef struct ModifyUserResponse {
    Status stat;
    char name[INFO_STRING_LEN];
    char uname[INFO_STRING_LEN];
    char password[INFO_STRING_LEN];
    int is_admin;
    id_t aid;
    id_t id;
} ModifyUserResponse;
#endif
